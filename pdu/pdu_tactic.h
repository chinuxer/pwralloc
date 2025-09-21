#ifndef PDUTACTIC_H
#define PDUTACTIC_H
#include "pdu_broker.h"

typedef struct
{
    uint8_t score;
    uint8_t node_id;
    uint8_t contactor_id;
} OPTIMAL;


void draw_pwrnode(int nodebool, bool post_scrip);
void draw_plug(int plug, bool post_scrip);
void draw_contactor(int contactor, bool post_scrip);
bool gear_insert(struct Alloc_plugObj *plug, struct Alloc_nodeObj *target);
#endif