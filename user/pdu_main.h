#ifndef PDUMAIN_H
#define PDUMAIN_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <ctype.h>
#include "inttypes.h"
#include "math.h"
#include "pdu_ex_datatype.h"

#define RTT_RX_BUFFER_SIZE (256)

#ifdef __DATABASE_IMPORT__
PowerDemand PwrDemandObj[24] = {0};
PowerSupply PwrSupplyObj[24] = {0};
Contactor ContactorObj[144] = {0};
#else
extern PowerDemand PwrDemandObj[24];
extern PowerSupply PwrSupplyObj[24];
extern Contactor ContactorObj[144];
#endif
#endif