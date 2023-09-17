# JSON files are run through customjson, which is a Pokabbie specific tool that converts JSON data to an output file

$(DATA_SRC_SUBDIR)/rogue/battle_music.h: $(DATA_SRC_SUBDIR)/rogue/battle_music.json
	$(CUSTOMJSON) battle_music_c $^ $@

$(DATA_SRC_SUBDIR)/rogue/trainers.h: $(DATA_SRC_SUBDIR)/rogue/trainers.json
	$(CUSTOMJSON) trainers_c $^ $@
