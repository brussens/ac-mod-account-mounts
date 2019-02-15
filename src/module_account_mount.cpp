/*
* Account share mounts module for AzerothCore ~2.0.
* copyright (c) since 2018 Dmitry Brusensky <brussens@nativeweb.ru>
* https://github.com/brussens/ac-account-mounts-module
*/
#include "Config.h"

class AccountMounts : public PlayerScript
{
public:
    AccountMounts() : PlayerScript("AccountMounts") { }

    void OnLogin(Player* player)
    {
       if (sConfigMgr->GetBoolDefault("Account.Mounts.Enable", true))
       {
            QueryResult guids = CharacterDatabase.PQuery(
                "SELECT `guid` FROM `characters` WHERE `account` = %u AND `guid` <> %u",
                player->GetSession()->GetAccountId(), player->GetGUID()
            );

            if (!guids)
                return;

            QueryResult spells = CharacterDatabase.PQuery("SELECT DISTINCT `spell` FROM `character_spell` WHERE `guid` IN(%s)", implodeGuids(guids).c_str());

            if (!spells)
                return;

            do {
                const SpellEntry* spell = sSpellStore.LookupEntry(spells->Fetch()[0].GetUInt32());

                if (!player->HasSpell(spell->Id) && isSpellCompatible(spell, player))
                    player->learnSpell(spell->Id);

            } while (spells->NextRow());
       }
	}
private:
    /*
    * Glue character guids in line for spell query.
    */
    std::string implodeGuids(QueryResult &queryResult)
    {
        uint8 i = 0;
        std::string condition;

        do {
            if (i)
                condition += ",";

            condition += std::to_string(queryResult->Fetch()[0].GetUInt32());
            ++i;
        } while (queryResult->NextRow());

        return condition;
    }

    /*
    * Check all compatibility for spell.
    */
    bool isSpellCompatible(const SpellEntry* spell, Player* player)
    {
        return isMount(spell) && isRaceCompatible(spell, player) && isClassCompatible(spell, player);
    }

    /*
    * Check spell on mount type.
    */
    bool isMount(const SpellEntry* spell)
    {
        return spell->Effect[0] == SPELL_EFFECT_APPLY_AURA && spell->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED;
    }

    /*
    * Check spell on race compatibility.
    */
    bool isRaceCompatible(const SpellEntry* spell, Player* player)
    {
        if (sConfigMgr->GetBoolDefault("Account.Mounts.StrictRace", true))
        {
            if (spell->AttributesEx7 & SPELL_ATTR7_ALLIANCE_ONLY)
            {
                return Player::TeamIdForRace(player->getRace()) == TEAM_ALLIANCE;
            }
            else if (spell->AttributesEx7 & SPELL_ATTR7_HORDE_ONLY)
            {
                return Player::TeamIdForRace(player->getRace()) == TEAM_HORDE;
            }
        }
        return true;
    }

    /*
    * Check spell on class compatibility.
    */
    bool isClassCompatible(const SpellEntry* spell, Player* player)
    {
        if (sConfigMgr->GetBoolDefault("Account.Mounts.StrictClass", true))
        {
            switch (spell->SpellFamilyName) {
                case SPELLFAMILY_PALADIN: return isValidClass(CLASS_PALADIN, player);
                case SPELLFAMILY_DEATHKNIGHT: return isValidClass(CLASS_DEATH_KNIGHT, player);
                case SPELLFAMILY_WARLOCK: return isValidClass(CLASS_WARLOCK, player);
                default: return true;
            }
        }
        return true;
    }

    bool isValidClass(uint8 const &ClassID, Player* player)
    {
        return player->getClass() == ClassID;
    }
};

class AccountMountsWorld : public WorldScript
{
public:
	AccountMountsWorld() : WorldScript("AccountMountsWorld") { }

	void OnBeforeConfigLoad(bool reload) override
	{
		if (!reload) {
			std::string conf_path = _CONF_DIR;
			std::string cfg_file = conf_path + "/module_account_mount.conf";
#ifdef WIN32
			cfg_file = "module_account_mount.conf";
#endif
			std::string cfg_def_file = cfg_file + ".dist";
			sConfigMgr->LoadMore(cfg_def_file.c_str());

			sConfigMgr->LoadMore(cfg_file.c_str());
		}
	}
};

void AddAccountMountsScripts()
{
    new AccountMountsWorld;
    new AccountMounts;
}
