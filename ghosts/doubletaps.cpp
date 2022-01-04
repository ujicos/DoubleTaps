#include "stdafx.hpp"
#include "doubletaps.hpp"

namespace component
{
	utils::hook::detour* pm_is_ads_allowed_hook = nullptr;
	utils::hook::detour* pm_weapon_process_hand_hook = nullptr;
	utils::hook::detour* pm_weapon_fire_weapon_hook = nullptr;

	int doubletaps::get_anim_for_hand(game::playerState_s* ps, int hand)
	{
		return ps->weaponStates[hand].weapAnim & 0xFF;
	}

	bool doubletaps::is_rechambering(game::playerState_s* ps, int hand)
	{
		return ps->weaponStates[hand].weaponstate == game::WEAPON_RECHAMBERING;
	}

	bool doubletaps::pm_is_ads_allowed_stub(game::pmove_t* pm)
	{
		if (get_anim_for_hand(pm->ps, 0) == game::WEAP_FORCE_IDLE && !pm->ps->isDualWielding)
		{
			return true;
		}

		return pm_is_ads_allowed_hook->original(pm);
	}

	void doubletaps::pm_weapon_process_hand_stub(game::pmove_t* pm, game::pml_t* pml, int delayedAction, int hand)
	{
		if (pm != nullptr && pm->ps != nullptr)
		{
			if (pm->ps->weaponStates[hand].weaponstate == game::WEAPON_FIRING)
			{
				pm_weapon_process_hand_hook->original(pm, pml, delayedAction, hand);
				return;
			}

			if (reinterpret_cast<int>(pm) < nullptr &&
				(pm->cmd.button & 0x1) &&
				(pm->cmd.button & 0x20) &&
				!pm->ps->isDualWielding &&
				!is_rechambering(pm->ps, hand) &&
				get_anim_for_hand(pm->ps, hand) == game::WEAP_FORCE_IDLE)
			{
				pm->ps->weaponStates[hand].weapAnim = game::BG_SegmentedReload(pm->ps->weapon, false) ? game::WEAP_RELOAD_START : game::WEAP_RELOAD;
				pm->ps->weaponFlags = 0;

				pm_weapon_fire_weapon_stub(pm, 0, &pml->holdrand, hand);
				return;
			}

			if (get_anim_for_hand(pm->ps, hand) == game::WEAP_FORCE_IDLE)
			{
				pm->ps->weaponStates[hand].weaponRestrictKickTime = 1;
				pm->ps->weaponStates[hand].weaponstate = game::WEAPON_READY;
				pm->ps->weaponStates[hand].weaponTime = 0;
				pm->ps->weaponStates[hand].weaponDelay = 0;
			}
		}

		pm_weapon_process_hand_hook->original(pm, pml, delayedAction, hand);
	}

	void doubletaps::pm_weapon_fire_weapon_stub(game::pmove_t* pm, int delayedAction, unsigned int* holdRand, int hand)
	{
		if (pm != nullptr && pm->ps != nullptr)
		{
			if (reinterpret_cast<int>(pm) < nullptr && (pm->cmd.button & 0x20) && game::PM_Weapon_AllowReload(pm->ps, hand))
			{
				if (get_anim_for_hand(pm->ps, hand) == game::WEAP_RELOAD || (get_anim_for_hand(pm->ps, hand) == game::WEAP_RELOAD_START))
					return;
			}
		}

		pm_weapon_fire_weapon_hook->original(pm, delayedAction, holdRand, hand);
	}

	void doubletaps::start()
	{
		pm_is_ads_allowed_hook = new utils::hook::detour(0x10BA8C, reinterpret_cast<void*>(pm_is_ads_allowed_stub));
		pm_weapon_process_hand_hook = new utils::hook::detour(0x117AA4, reinterpret_cast<void*>(pm_weapon_process_hand_stub));
		pm_weapon_fire_weapon_hook = new utils::hook::detour(0x1150F0, reinterpret_cast<void*>(pm_weapon_fire_weapon_stub));
	}

	void doubletaps::stop()
	{
		delete pm_is_ads_allowed_hook;
		delete pm_weapon_process_hand_hook;
		delete pm_weapon_fire_weapon_hook;
	}
}