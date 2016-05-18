void ufraw_kelvin_green_to_multipliers(double temperature, double green, double chanMulArray[3]);
void ufraw_multipliers_to_kelvin_green(double chanMulArray[3], double* temperature, double* green);

/* call this at startup to set the color matrix */
void adobe_coeff (const char *make, const char *model);
