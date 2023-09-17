#include "BakeHelpers.h"

#include <string>
#include <fstream>
#include <array>
#include <vector>
#include <set>

extern "C"
{
	#include "rogue_baked.h"

	extern const struct BaseStats gBaseStats[];

	extern const struct RoguePokedexVariant gPokedexVariants[POKEDEX_VARIANT_COUNT];
	extern const struct RoguePokedexRegion gPokedexRegions[POKEDEX_REGION_COUNT];
}

u16 eggLookup[NUM_SPECIES]{ SPECIES_NONE };
u8 evolutionCountLookup[NUM_SPECIES]{ 0 };

std::set<u16> eggEvolutionTypes[NUM_SPECIES];

static bool HasEvolutionConnectionOfType(u16 species, u16 type)
{
	u16 eggSpecies = eggLookup[species];
	auto types = eggEvolutionTypes[eggSpecies];

	return types.find(type) != types.end();
}

static void SetBitFlag(u16 elem, bool state, u8* arr)
{
	u16 idx = elem / 8;
	u16 bit = elem % 8;

	u8 bitMask = 1 << bit;

	if (state)
	{
		arr[idx] |= bitMask;
	}
	else
	{
		arr[idx] &= ~bitMask;
	}
}

static bool GetBitFlag(u16 elem, u8* arr)
{
	u16 idx = elem / 8;
	u16 bit = elem % 8;

	u8 bitMask = 1 << bit;

	return (arr[idx] & bitMask) != 0;
}

int main()
{
	for (int s = SPECIES_NONE; s < NUM_SPECIES; ++s)
	{
		if (s == SPECIES_NONE)
		{
			eggLookup[s] = s;
			evolutionCountLookup[s] = 0;
		}
		else
		{
			u16 eggSpecies = Rogue_GetEggSpecies(s);
			eggLookup[s] = eggSpecies;
			evolutionCountLookup[s] = Rogue_GetEvolutionCount(s);

			if (gBaseStats[s].type1 != TYPE_NONE)
				eggEvolutionTypes[eggSpecies].insert(gBaseStats[s].type1);

			if (gBaseStats[s].type2 != TYPE_NONE)
				eggEvolutionTypes[eggSpecies].insert(gBaseStats[s].type2);
		}
	}

	std::string const c_OutputPath = "..\\..\\..\\..\\src\\data\\rogue_bake_data.h";

	std::ofstream file;
	file.open(c_OutputPath, std::ios::out);

	file << "// == WARNING ==\n";
	file << "// DO NOT EDIT THIS FILE\n";
	file << "// This file was automatically generated by PokemonQueryBaker\n";
	file << "\n";

	// Egg Species
	//
	{
		file << "const u16 gRogueBake_EggSpecies[NUM_SPECIES] =\n{\n";
		for (int s = SPECIES_NONE; s < NUM_SPECIES; ++s)
		{
			file << "\t[" << s << "] = " << (int)eggLookup[s] << ",\n";
		}
		file << "};\n";
		file << "\n";
	}

	// Evo Counts
	//
	{
		file << "const u8 gRogueBake_EvolutionCount[NUM_SPECIES] =\n{\n";
		for (int s = SPECIES_NONE; s < NUM_SPECIES; ++s)
		{
			file << "\t[" << s << "] = " << (int)evolutionCountLookup[s] << ",\n";
		}
		file << "};\n";

		file << "\n";
	}

	// Evo of type
	//
	{
		file << "const u32 gRogueBake_EvolutionChainTypeFlags[NUM_SPECIES] =\n{\n";
		for (int s = SPECIES_NONE; s < NUM_SPECIES; ++s)
		{
			u32 typeFlags = 0;

			for (int t = 0; t < NUMBER_OF_MON_TYPES; ++t)
			{
				if (HasEvolutionConnectionOfType(s, t))
					typeFlags |= MON_TYPE_VAL_TO_FLAGS(t);
			}

			file << "\t[" << s << "] = " << (int)typeFlags << ",\n";
		}
		file << "};\n";
		file << "\n";
	}

	// Dex variant lists
	//
	{
		std::array<u8, SPECIES_FLAGS_BYTE_COUNT> bitFlags;
		bitFlags.fill(0);

		file << "\n";
		file << "const u8 gRogueBake_PokedexVariantBitFlags[POKEDEX_VARIANT_COUNT][SPECIES_FLAGS_BYTE_COUNT] =\n{\n";

		for (int i = 0; i < POKEDEX_VARIANT_COUNT; ++i)
		{
			bitFlags.fill(0);

			for (int j = 0; j < gPokedexVariants[i].speciesCount; ++j)
				SetBitFlag(gPokedexVariants[i].speciesList[j], TRUE, &bitFlags[0]);

			file << "\t[" << i << "] = \n\t{" << "\n";

			for (int j = 0; j < SPECIES_FLAGS_BYTE_COUNT; ++j)
			{
				file << "\t\t" << (int)bitFlags[j] << ",\n";
			}

			file << "\t},\n";
		}

		//extern const struct RoguePokedexVariant gPokedexVariants[POKEDEX_VARIANT_COUNT];
		//extern const struct RoguePokedexRegion gPokedexRegions[POKEDEX_REGION_COUNT];

		file << "};\n";
	}

	file.close();
	return 0;
}
