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
#include "stm32f4xx.h"
#include "SEGGER_RTT.h"

#define UPPER_LIMIT_OF_PLUGS 24
#define UPPER_LIMIT_OF_NODES 24
#define UPPER_LIMIT_OF_CONTACTORS 144
void print_oneliner(const char *format, ...);
#ifdef __DATABASE_IMPORT__

PowerDemand PwrDemandObj[UPPER_LIMIT_OF_PLUGS] = {0};
PowerSupply PwrSupplyObj[UPPER_LIMIT_OF_NODES] = {0};
Contactor ContactorObj[UPPER_LIMIT_OF_CONTACTORS] = {0};
#else
extern PowerDemand PwrDemandObj[UPPER_LIMIT_OF_PLUGS];
extern PowerSupply PwrSupplyObj[UPPER_LIMIT_OF_NODES];
extern Contactor ContactorObj[UPPER_LIMIT_OF_CONTACTORS];
#endif
#endif