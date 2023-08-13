#ifndef QUEST_POPUP_H
#define QUEST_POPUP_H

void Rogue_ClearPopupQueue(void);
void Rogue_UpdatePopups(bool8 inOverworld, bool8 inputEnabled);

void Rogue_PushPopup_PartyNotifications();
void Rogue_PushPopup_NewMoves(u8 slotId);
void Rogue_PushPopup_NewEvos(u8 slotId);
void Rogue_PushPopup_UnableToEvolve(u8 slotId);

void Rogue_PushPopup_QuestComplete(u16 questId);
void Rogue_PushPopup_QuestFail(u16 questId);

void Rogue_PushPopup_PokemonChain(u16 species, u16 chainSize);
void Rogue_PushPopup_PokemonChainBroke(u16 species);

void Rogue_PushPopup_WeakPokemonClause(u16 species);
void Rogue_PushPopup_StrongPokemonClause(u16 species);

#endif //QUEST_POPUP_H
