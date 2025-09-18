param_ref = ADD_KEYVALUE({"u32gun01", 1}, {"u32gun01", 17}, {"u32gun02", 2}, {"u32gun02", 18}, {"u32gun03", 3}, {"u32gun03", 19});
param_ref = ADD_KEYVALUE({"u32gun04", 4}, {"u32gun05", 20}, {"u32gun06", 33}, {"u32gun06", 49}, {"u32gun07", 34}, {"u32gun08", 35});
param_ref = ADD_KEYVALUE({"u32gun09", 36}, {"u32gun10", 50}, {"u32gun11", 51}, {"u32gun12", 52});
uint32_t u32pwrnodes_max = 16;
uint32_t u32pwrcontactors_max = 64;
uint32_t u32pool_max = 4;
uint32_t u32pwrguns_max = 12;
param_ref = ADD_INTEGER(u32pwrnodes_max, u32pwrguns_max, u32pwrcontactors_max, u32pool_max);
