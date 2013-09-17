/* Dual ISO interface */

extern WEAK_FUNC(ret_0) int dual_iso_is_enabled();

extern WEAK_FUNC(ret_0) int dual_iso_get_recovery_iso(); /* raw iso values */

extern WEAK_FUNC(ret_0) int dual_iso_set_recovery_iso(int raw_iso);

extern WEAK_FUNC(ret_0) int dual_iso_calc_dr_improvement(int iso1, int iso2); /* ev x100 */

extern WEAK_FUNC(ret_0) int dual_iso_get_dr_improvement(); /* with current settings */
