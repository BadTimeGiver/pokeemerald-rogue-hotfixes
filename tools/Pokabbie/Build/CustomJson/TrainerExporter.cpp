#include "main.h"

#include <sstream>

struct TrainerDataExport_C
{
	std::stringstream earlyBlock;
	std::stringstream trainerStructsBlock;
};

struct TrainerStrings
{
	std::vector<std::string> text;
	std::unordered_map<std::string, int> textToIndex;
};

static void ExportTrainerGroupData_C(TrainerDataExport_C& exporter, json const& jsonData, std::string trainerGroup);
static void ExportQueryScriptData_C(TrainerDataExport_C& exporter, std::string const& exportName, json const& jsonData);

static TrainerStrings ExtractTrainerStrings(json const& trainers);
static void ExportTrainerStringsData_C(TrainerDataExport_C& exporter, json const& trainers);

void ExportTrainerData_C(std::ofstream& fileStream, json const& jsonData)
{
	TrainerDataExport_C exporter;

	json trainers = jsonData["trainers"];
	ExportTrainerStringsData_C(exporter, trainers);

	for (auto it = trainers.begin(); it != trainers.end(); ++it)
	{
		ExportTrainerGroupData_C(exporter, jsonData, it.key());
	}

	fileStream << exporter.earlyBlock.str() << '\n';

	fileStream << "const struct RogueTrainer gRogueTrainers" << "[] = \n{\n";
	fileStream << c_TabSpacing << "{}, // TRAINER_NONE\n";
	fileStream << exporter.trainerStructsBlock.str() << '\n';
	fileStream << "};\n\n";
	fileStream << "const u16 gRogueTrainerCount = ARRAY_COUNT(gRogueTrainers);\n";
}

static void ExportTrainerGroupData_C(TrainerDataExport_C& exporter, json const& jsonData, std::string trainerGroup)
{
	TrainerStrings trainerStrings = ExtractTrainerStrings(jsonData["trainers"]);
	json trainers = jsonData["trainers"][trainerGroup];
	int i;

	std::string trainerGroupPrefix = "// START " + trainerGroup + "\n";
	std::string trainerGroupSuffix = "// END " + trainerGroup + "\n";

	if (strutil::starts_with(trainerGroup, "#if"))
	{
		trainerGroupPrefix = trainerGroup + "\n";
		trainerGroupSuffix = "#endif\n";

		strutil::replace_all(trainerGroup, "#if", "");
	}

	// Ensure trainerGroup is safe to use as a codeToken
	strutil::replace_all(trainerGroup, " ", "_");
	strutil::replace_all(trainerGroup, "//", "_");

	exporter.earlyBlock << trainerGroupPrefix;
	exporter.trainerStructsBlock << trainerGroupPrefix;

	// Names
	i = 0;
	for (auto trainer : trainers)
	{
		int trainerIdx = i++;
		exporter.earlyBlock << "static const u8 sTrainerName_" << trainerGroup << "_" << trainerIdx << "[] = _(\"" << trainer["name"].get<std::string>() << "\");\n";

		if (trainer.contains("encounter_text"))
		{
			auto encounterTextArray = trainer["encounter_text"];
			exporter.earlyBlock << "static const u8* const sTrainerEncounterText_" << trainerGroup << "_" << trainerIdx << "[TRAINER_STRING_COUNT * " << encounterTextArray.size() << "] = \n{\n";

			int entryIdx = 0;
			for (auto encounterText : encounterTextArray)
			{
				for (auto entryIt = encounterText.begin(); entryIt != encounterText.end(); ++entryIt)
				{
					std::string key = entryIt.key();
					std::string text = entryIt.value().get<std::string>();
					int textIndex = trainerStrings.textToIndex[text];

					exporter.earlyBlock << c_TabSpacing << "[TRAINER_STRING_COUNT * " << entryIdx  << " + TRAINER_STRING_" << key << "] = gTrainerEncounterText_" << textIndex << ",\n";
				}
				++entryIdx;
			}

			exporter.earlyBlock << "};\n";
		}
	}
	exporter.earlyBlock << "\n";

	// Generator scripts
	i = 0;
	for (auto trainer : trainers)
	{
		int trainerIdx = i++;
		json generator = trainer["team_generator"];
		std::string trainerSuffix = trainerGroup + "_" + std::to_string(trainerIdx);

		if (generator.contains("query_script_override"))
		{
			ExportQueryScriptData_C(
				exporter,
				"sTrainerQueryScriptOverride_" + trainerSuffix,
				generator["query_script_override"]
			);
		}

		if (generator.contains("query_script_post"))
		{
			ExportQueryScriptData_C(
				exporter,
				"sTrainerQueryScriptPost_" + trainerSuffix,
				generator["query_script_post"]
			);
		}

		if (generator.contains("weight_script"))
		{
			ExportQueryScriptData_C(
				exporter,
				"sTrainerWeightScript_" + trainerSuffix,
				generator["weight_script"]
			);
		}

		// Subsets
		{
			json subsets = generator["subsets"];

			exporter.earlyBlock << "\nstatic const struct RogueTeamGeneratorSubset sTrainerTeamSubsets_" << trainerSuffix << "[] =\n{\n";
			for (auto subset : subsets)
			{
				exporter.earlyBlock << c_TabSpacing << "{\n";
				exporter.earlyBlock << c_TabSpacing << ".maxSamples = " << subset["max_samples"].get<int>() << ",\n";

				// included types
				exporter.earlyBlock << c_TabSpacing << ".includedTypeMask = 0";
				if (subset.contains("include_types"))
				{
					for (auto type : subset["include_types"])
						exporter.earlyBlock << " | MON_TYPE_VAL_TO_FLAGS(TYPE_" << type.get<std::string>() << ")";
				}
				exporter.earlyBlock << ",\n";

				// excluded types
				exporter.earlyBlock << c_TabSpacing << ".excludedTypeMask = 0";
				if (subset.contains("exclude_types"))
				{
					for (auto type : subset["exclude_types"])
						exporter.earlyBlock << " | MON_TYPE_VAL_TO_FLAGS(TYPE_" << type.get<std::string>() << ")";
				}
				exporter.earlyBlock << ",\n";

				exporter.earlyBlock << c_TabSpacing << "},\n";

			}
			exporter.earlyBlock << "};\n";
		}
	}
	exporter.earlyBlock << "\n";

	// Generators

	// Trainer data
	i = 0;
	for (auto trainer : trainers)
	{
		int trainerIdx = i++;
		std::string trainerSuffix = trainerGroup + "_" + std::to_string(trainerIdx);

		exporter.trainerStructsBlock << c_TabSpacing << "{\n";

		exporter.trainerStructsBlock << c_TabSpacing << ".trainerName = sTrainerName_" << trainerSuffix << ",\n";
		exporter.trainerStructsBlock << c_TabSpacing << ".trainerClass = " << trainer["trainer_class"].get<std::string>() << ",\n";
		exporter.trainerStructsBlock << c_TabSpacing << ".musicPlayer = BATTLE_MUSIC_" << strutil::to_upper(trainer["music_player"].get<std::string>()) << ",\n";
		exporter.trainerStructsBlock << c_TabSpacing << ".typeAssignment = TYPE_" << strutil::to_upper(trainer["type_assignment"].get<std::string>()) << ",\n";

		if (trainer.contains("level_override"))
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".levelOverride = " << trainer["level_override"].get<int>() << ",\n";
		}
		else
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".levelOverride = 0,\n";
		}
		
		if (trainer.contains("type_group_override"))
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".typeAssignmentGroup = TYPE_" << strutil::to_upper(trainer["type_group_override"].get<std::string>()) << ",\n";
		}
		else
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".typeAssignmentGroup = TYPE_" << strutil::to_upper(trainer["type_assignment"].get<std::string>()) << ",\n";
		}

		if (trainer.contains("gfx_suffix"))
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".objectEventGfx = OBJ_EVENT_GFX_" << trainer["gfx_suffix"].get<std::string>() << ",\n";
			exporter.trainerStructsBlock << c_TabSpacing << ".trainerPic = TRAINER_PIC_" << trainer["gfx_suffix"].get<std::string>() << ",\n";
		}
		else
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".objectEventGfx = " << trainer["object_event"].get<std::string>() << ",\n";
			exporter.trainerStructsBlock << c_TabSpacing << ".trainerPic = " << trainer["trainer_pic"].get<std::string>() << ",\n";
		}

		// Weather
		if(trainer.contains("weather"))
			exporter.trainerStructsBlock << c_TabSpacing << ".preferredWeather = WEATHER_" << strutil::to_upper(trainer["weather"].get<std::string>()) << ",\n";
		else
			exporter.trainerStructsBlock << c_TabSpacing << ".preferredWeather = WEATHER_DEFAULT,\n";

		// Flags
		exporter.trainerStructsBlock << c_TabSpacing << ".trainerFlags = TRAINER_FLAG_NONE";
		for (auto flag : trainer["trainer_flags"])
		{
			exporter.trainerStructsBlock << " | TRAINER_FLAG_" << strutil::to_upper(flag.get<std::string>());
		}
		exporter.trainerStructsBlock << ",\n";

		// Strings
		if (trainer.contains("encounter_text"))
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".encounterText = sTrainerEncounterText_" << trainerGroup << "_" << trainerIdx << ",\n";
			exporter.trainerStructsBlock << c_TabSpacing << ".encounterTextCount = ARRAY_COUNT(sTrainerEncounterText_" << trainerGroup << "_" << trainerIdx << ") / TRAINER_STRING_COUNT,\n";
		}
		else
		{
			exporter.trainerStructsBlock << c_TabSpacing << ".encounterText = NULL,\n";
			exporter.trainerStructsBlock << c_TabSpacing << ".encounterTextCount = 0,\n";
		}


		// Team generator
		exporter.trainerStructsBlock << c_TabSpacing << ".teamGenerator = \n" << c_TabSpacing << "{\n";
		{
			json generator = trainer["team_generator"];

			if (generator.contains("query_script_override"))
				exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".queryScriptOverride = " << "sTrainerQueryScriptOverride_" << trainerSuffix << ",\n";
			else
				exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".queryScriptOverride = NULL,\n";

			if (generator.contains("query_script_post"))
				exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".queryScriptPost = " << "sTrainerQueryScriptPost_" << trainerSuffix << ",\n";
			else
				exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".queryScriptPost = NULL,\n";

			if (generator.contains("weight_script"))
				exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".weightScript = " << "sTrainerWeightScript_" << trainerSuffix << ",\n";
			else
				exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".weightScript = NULL,\n";

			exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".subsets = sTrainerTeamSubsets_" << trainerSuffix << ", \n";
			exporter.trainerStructsBlock << c_TabSpacing << c_TabSpacing << ".subsetCount = ARRAY_COUNT(sTrainerTeamSubsets_" << trainerSuffix << "), \n";
		}
		exporter.trainerStructsBlock << c_TabSpacing << "},\n";

		exporter.trainerStructsBlock << c_TabSpacing << "},\n";
	}

	exporter.earlyBlock << trainerGroupSuffix;
	exporter.trainerStructsBlock << trainerGroupSuffix;
}

static void ExportQueryScriptData_C(TrainerDataExport_C& exporter, std::string const& exportName, json const& jsonData)
{
	exporter.earlyBlock << "static const u16 " << exportName << "[] =\n{\n";

	for (auto cmdEntry : jsonData)
	{
		std::string cmd = cmdEntry.get<std::string>();

		if (strutil::starts_with(cmd, "TYPE_")
			|| strutil::starts_with(cmd, "STAT_")
			|| strutil::starts_with(cmd, "SPECIES_")
			|| strutil::starts_with(cmd, "ITEM_")
			)
		{
			// do nothing, these are allowed constants we can reference
		}
		else if (strutil::starts_with(cmd, "VAR_"))
		{
			cmd = "QUERY_MASK_VAR_LOOKUP | QUERY_" + cmd;
		}
		else if (std::to_string(strutil::parse_string<int>(cmd)) == cmd)
		{
			// do nothing, this is a number
		}
		else
		{
			// Append query script, as this makes the json's slightly cleaner
			cmd = "QUERY_SCRIPT_" + cmd;
		}

		exporter.earlyBlock << c_TabSpacing << cmd << ",\n";
	}

	exporter.earlyBlock << c_TabSpacing << "QUERY_SCRIPT_END\n};\n";
}

void ExportTrainerData_Pory(std::ofstream& fileStream, json const& jsonData)
{
	TrainerStrings trainerStrings = ExtractTrainerStrings(jsonData["trainers"]);

	for (size_t i = 0; i < trainerStrings.text.size(); ++i)
	{
		fileStream << "text gTrainerEncounterText_" << i << "\n{\n";
		fileStream << c_TabSpacing << "format(\"" << trainerStrings.text[i] << "\")\n";
		fileStream << "}\n\n";
	}
}

static TrainerStrings ExtractTrainerStrings(json const& trainers)
{
	TrainerStrings trainerStrings;

	for (auto groupIt = trainers.begin(); groupIt != trainers.end(); ++groupIt)
	{
		for (auto trainer : groupIt.value())
		{
			if (trainer.contains("encounter_text"))
			{
				for (auto encounterText : trainer["encounter_text"])
				{
					for (auto entryIt = encounterText.begin(); entryIt != encounterText.end(); ++entryIt)
					{
						std::string text = entryIt.value().get<std::string>();
						auto findIt = trainerStrings.textToIndex.find(text);

						if (findIt == trainerStrings.textToIndex.end())
						{
							trainerStrings.textToIndex[text] = trainerStrings.text.size();
							trainerStrings.text.push_back(text);
						}
					}
				}
			}
		}
	}

	return trainerStrings;
}

static void ExportTrainerStringsData_C(TrainerDataExport_C& exporter, json const& trainers)
{
	TrainerStrings trainerStrings = ExtractTrainerStrings(trainers);

	for (size_t i = 0; i < trainerStrings.text.size(); ++i)
	{
		exporter.earlyBlock << "extern const u8 gTrainerEncounterText_" << i << "[];\n";
	}
	exporter.earlyBlock << "\n";
}