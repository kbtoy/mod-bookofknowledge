#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "SharedDefines.h"
#include "DatabaseEnv.h"
#include "ScriptedGossip.h" 
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include <string>
#include <sstream>

#define BOOK_OF_KNOWLEDGE_ID 16073 

enum BookOfKnowledgeStatType
{
    BOK_STAT_BANKED         = 0,
    BOK_STAT_BLOCK          = 1,
    BOK_STAT_DODGE          = 2,
    BOK_STAT_PARRY          = 3,
    BOK_STAT_HASTE          = 4,
    BOK_STAT_HIT            = 5,
    BOK_STAT_CRIT           = 6,
    BOK_STAT_EXPERTISE      = 7,
    BOK_STAT_ATTACK_POWER   = 8,
    BOK_STAT_DEFENSE        = 9,
    BOK_STAT_STRENGTH       = 10,
    BOK_STAT_STAMINA        = 11,
    BOK_STAT_INTELLECT      = 12,
    BOK_STAT_AGILITY        = 13,
    BOK_STAT_SPELL_POWER    = 14,
    BOK_STAT_SPELL_HIT      = 15,
    BOK_STAT_SPELL_CRIT     = 16,
    BOK_STAT_SPELL_HASTE    = 17
};

enum LibrarianActions
{
    ACTION_OPEN_UI          = 1001,
    ACTION_RESET_STATS      = 1002
};

// --- HELPER FUNCTIONS ---

std::string GetStatConfigName(uint32 statType)
{
    switch (statType)
    {
        case BOK_STAT_BLOCK: return "Block";
        case BOK_STAT_DODGE: return "Dodge";
        case BOK_STAT_PARRY: return "Parry";
        case BOK_STAT_HASTE: return "Haste";
        case BOK_STAT_HIT: return "Hit";
        case BOK_STAT_CRIT: return "Crit";
        case BOK_STAT_EXPERTISE: return "Expertise";
        case BOK_STAT_ATTACK_POWER: return "AttackPower";
        case BOK_STAT_DEFENSE: return "Defense";
        case BOK_STAT_STRENGTH: return "Strength";
        case BOK_STAT_STAMINA: return "Stamina";
        case BOK_STAT_INTELLECT: return "Intellect";
        case BOK_STAT_AGILITY: return "Agility";
        case BOK_STAT_SPELL_POWER: return "SpellPower";
        case BOK_STAT_SPELL_HIT: return "SpellHit";
        case BOK_STAT_SPELL_CRIT: return "SpellCrit";
        case BOK_STAT_SPELL_HASTE: return "SpellHaste";
        default: return "";
    }
}

void ApplyAuraStat(Player* player, uint32 spellId, int32 amount, bool apply)
{
    player->RemoveAura(spellId); 
    if (apply && amount > 0)
        player->CastCustomSpell(player, spellId, &amount, &amount, &amount, true);
}

void ApplyCustomBookOfKnowledgeStat(Player* player, uint32 statType, int32 amount, bool apply)
{
    if (!player) return;

    switch (statType)
    {
        case BOK_STAT_STRENGTH:      ApplyAuraStat(player, 7464, amount, apply); break;
        case BOK_STAT_AGILITY:       ApplyAuraStat(player, 7471, amount, apply); break;
        case BOK_STAT_STAMINA:       ApplyAuraStat(player, 7477, amount, apply); break;
        case BOK_STAT_INTELLECT:     ApplyAuraStat(player, 7468, amount, apply); break;
        case BOK_STAT_SPELL_POWER:   ApplyAuraStat(player, 18030, amount, apply); break;
        case BOK_STAT_ATTACK_POWER:  ApplyAuraStat(player, 15809, amount, apply); break;
    }

    switch (statType)
    {
        case BOK_STAT_BLOCK:     player->ApplyRatingMod(CR_BLOCK, amount, apply); break;
        case BOK_STAT_DODGE:     player->ApplyRatingMod(CR_DODGE, amount, apply); break;
        case BOK_STAT_PARRY:     player->ApplyRatingMod(CR_PARRY, amount, apply); break;
        case BOK_STAT_EXPERTISE: player->ApplyRatingMod(CR_EXPERTISE, amount, apply); break;
        case BOK_STAT_DEFENSE:   player->ApplyRatingMod(CR_DEFENSE_SKILL, amount, apply); break;
        case BOK_STAT_HIT:       player->ApplyRatingMod(CR_HIT_MELEE, amount, apply); player->ApplyRatingMod(CR_HIT_RANGED, amount, apply); break;
        case BOK_STAT_CRIT:      player->ApplyRatingMod(CR_CRIT_MELEE, amount, apply); player->ApplyRatingMod(CR_CRIT_RANGED, amount, apply); break;
        case BOK_STAT_HASTE:     player->ApplyRatingMod(CR_HASTE_MELEE, amount, apply); player->ApplyRatingMod(CR_HASTE_RANGED, amount, apply); break;
        case BOK_STAT_SPELL_HIT: player->ApplyRatingMod(CR_HIT_SPELL, amount, apply); break;
        case BOK_STAT_SPELL_CRIT:player->ApplyRatingMod(CR_CRIT_SPELL, amount, apply); break;
        case BOK_STAT_SPELL_HASTE:player->ApplyRatingMod(CR_HASTE_SPELL, amount, apply); break;
    }

    player->UpdateAllStats();
    player->UpdateAttackPowerAndDamage(false);
    player->UpdateAttackPowerAndDamage(true);
}

// Packages all the player's data into a compressed string and shoots it to the Lua Addon
void SyncAddonData(Player* player)
{
    uint32 bankedPoints = 0;
    std::map<uint32, uint32> investedStats;

    QueryResult result = CharacterDatabase.Query("SELECT stat_type, amount FROM character_bookofknowledge WHERE guid = {}", player->GetGUID().GetCounter());
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 type = fields[0].Get<uint8>();
            uint32 amount = fields[1].Get<uint32>();

            if (type == BOK_STAT_BANKED) bankedPoints = amount;
            else investedStats[type] = amount;
        } while (result->NextRow());
    }

    // Build the sync string: [BOK_SYNC]:Banked:BooksInBags:Stat1Invested:Stat1Max:Stat2Invested...
    std::stringstream syncString;
    syncString << "[BOK_SYNC]:" << bankedPoints << ":" << player->GetItemCount(BOOK_OF_KNOWLEDGE_ID);

    for (uint32 type = BOK_STAT_BLOCK; type <= BOK_STAT_SPELL_HASTE; ++type)
    {
        std::string configName = GetStatConfigName(type);
        uint32 maxCap = sConfigMgr->GetOption<int32>("Book of Knowledge.Max." + configName, 100);
        uint32 enabled = sConfigMgr->GetOption<bool>("Book of Knowledge.Stat." + configName, true) ? 1 : 0;
        
        syncString << ":" << type << ":" << enabled << ":" << investedStats[type] << ":" << maxCap;
    }

    ChatHandler(player->GetSession()).SendSysMessage(syncString.str().c_str());
}

// --- ADDON COMMUNICATION LISTENER ---

class Bookofknowledge_Addon_Listener : public PlayerScript
{
public:
    Bookofknowledge_Addon_Listener() : PlayerScript("Bookofknowledge_Addon_Listener") {}

// Helper function to process the secret whispers
    bool ProcessAddonCommand(Player* player, std::string& msg)
    {
        if (msg.find("BOK_CMD:") == 0)
        {
            // 1. Turn In Books
            if (msg == "BOK_CMD:TURN_IN")
            {
                uint32 booksToDeposit = player->GetItemCount(BOOK_OF_KNOWLEDGE_ID);
                if (booksToDeposit > 0)
                {
                    player->DestroyItemCount(BOOK_OF_KNOWLEDGE_ID, booksToDeposit, true);
                    CharacterDatabase.Execute("INSERT INTO character_bookofknowledge (guid, stat_type, amount) VALUES ({}, 0, {}) ON DUPLICATE KEY UPDATE amount = amount + {}", player->GetGUID().GetCounter(), booksToDeposit, booksToDeposit);
                }
                return false; // suppress the whisper
            }
            
            // 2. Apply Points
            if (msg.find("BOK_CMD:APPLY:") == 0)
            {
                int statType, amountToSpend;
                if (sscanf(msg.c_str(), "BOK_CMD:APPLY:%d:%d", &statType, &amountToSpend) == 2 && amountToSpend > 0)
                {
                    QueryResult bankResult = CharacterDatabase.Query("SELECT amount FROM character_bookofknowledge WHERE guid = {} AND stat_type = 0", player->GetGUID().GetCounter());
                    if (bankResult && bankResult->Fetch()[0].Get<uint32>() >= (uint32)amountToSpend)
                    {
                        uint32 oldTotal = 0;
                        QueryResult oldRes = CharacterDatabase.Query("SELECT amount FROM character_bookofknowledge WHERE guid = {} AND stat_type = {}", player->GetGUID().GetCounter(), statType);
                        if (oldRes) oldTotal = oldRes->Fetch()[0].Get<uint32>();
                        
                        uint32 newTotal = oldTotal + amountToSpend;
                        ApplyCustomBookOfKnowledgeStat(player, statType, oldTotal, false);
                        ApplyCustomBookOfKnowledgeStat(player, statType, newTotal, true);
                        
                        CharacterDatabase.Execute("INSERT INTO character_bookofknowledge (guid, stat_type, amount) VALUES ({}, {}, {}) ON DUPLICATE KEY UPDATE amount = {}", player->GetGUID().GetCounter(), statType, newTotal, newTotal);
                        CharacterDatabase.Execute("UPDATE character_bookofknowledge SET amount = amount - {} WHERE guid = {} AND stat_type = 0", amountToSpend, player->GetGUID().GetCounter());
                    }
                }
                return false; // suppress the whisper
            }

            if (msg == "BOK_CMD:SYNC")
            {
                SyncAddonData(player);
                return false;
            }
        }
        
        return true; // not our command, let it through normally
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 /*language*/, std::string& msg, Player* /*receiver*/) override
    {
        if (type == CHAT_MSG_WHISPER)
            ProcessAddonCommand(player, msg);
        return true; 
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!sConfigMgr->GetOption<bool>("Book of Knowledge.Enable", false)) return;

        QueryResult result = CharacterDatabase.Query("SELECT stat_type, amount FROM character_bookofknowledge WHERE guid = {}", player->GetGUID().GetCounter());
        if (!result) return;
        do {
            Field* fields = result->Fetch();
            if (fields[0].Get<uint8>() != BOK_STAT_BANKED)
                ApplyCustomBookOfKnowledgeStat(player, fields[0].Get<uint8>(), fields[1].Get<int32>(), true);
        } while (result->NextRow());
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* /*quest*/) override
    {
        if (sConfigMgr->GetOption<bool>("Book of Knowledge.Enable", false) && sConfigMgr->GetOption<bool>("Book of Knowledge.Book.Quest.Enable", true))
            if (roll_chance_f(sConfigMgr->GetOption<float>("Book of Knowledge.Book.Quest.Chance", 10.0f)))
                player->AddItem(BOOK_OF_KNOWLEDGE_ID, 1);
    }

    void OnPlayerAfterCreatureLoot(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("Book of Knowledge.Enable", false) && 
            sConfigMgr->GetOption<bool>("Book of Knowledge.Book.Loot.Enable", true))
            if (roll_chance_f(sConfigMgr->GetOption<float>("Book of Knowledge.Book.Loot.Chance", 5.0f)))
                player->AddItem(BOOK_OF_KNOWLEDGE_ID, 1);
    }
};

// --- CREATURE SCRIPT: The Librarian NPC ---

class Bookofknowledge_Librarian_Gossip : public CreatureScript
{
public:
    Bookofknowledge_Librarian_Gossip() : CreatureScript("Bookofknowledge_Librarian_Gossip") {}

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sConfigMgr->GetOption<bool>("Book of Knowledge.Enable", false)) return false;

        ClearGossipMenuFor(player);

        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Allocate Book of Knowledge Points", GOSSIP_SENDER_MAIN, ACTION_OPEN_UI);

        // NATIVE WOW GOLD POPUP
        uint32 resetCost = sConfigMgr->GetOption<int32>("Book of Knowledge.ResetCost", 2500000);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Reset All Knowledge Points", GOSSIP_SENDER_MAIN, ACTION_RESET_STATS, "Are you sure you want to reset all allocated Knowledge Points?", resetCost, false);

        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        if (action == ACTION_OPEN_UI)
        {
            SyncAddonData(player); // Shoot the data to the Lua Addon
            CloseGossipMenuFor(player); // The UI is open, close the NPC menu
            return true;
        }

        if (action == ACTION_RESET_STATS)
        {
            uint32 resetCost = sConfigMgr->GetOption<int32>("Book of Knowledge.ResetCost", 2500000);
            if (!player->HasEnoughMoney(resetCost))
            {
                ChatHandler(player->GetSession()).SendSysMessage("You do not have enough gold to reset your points.");
                CloseGossipMenuFor(player);
                return false;
            }

            uint32 totalRefund = 0;
            QueryResult res = CharacterDatabase.Query("SELECT stat_type, amount FROM character_bookofknowledge WHERE guid = {} AND stat_type != 0", player->GetGUID().GetCounter());
            if (res)
            {
                do {
                    totalRefund += res->Fetch()[1].Get<int32>();
                    ApplyCustomBookOfKnowledgeStat(player, res->Fetch()[0].Get<uint8>(), res->Fetch()[1].Get<int32>(), false);
                } while (res->NextRow());
            }

            if (totalRefund > 0)
            {
                player->ModifyMoney(-resetCost);
                CharacterDatabase.Execute("INSERT INTO character_bookofknowledge (guid, stat_type, amount) VALUES ({}, 0, {}) ON DUPLICATE KEY UPDATE amount = amount + {}", player->GetGUID().GetCounter(), totalRefund, totalRefund);
                CharacterDatabase.Execute("DELETE FROM character_bookofknowledge WHERE guid = {} AND stat_type != 0", player->GetGUID().GetCounter());
                ChatHandler(player->GetSession()).SendSysMessage("Knowledge points successfully reset!");
            }
            CloseGossipMenuFor(player);
            return true;
        }
        return false;
    }
};

void SC_AddBookofknowledgeScripts()
{
    new Bookofknowledge_Addon_Listener();
    new Bookofknowledge_Librarian_Gossip();
}