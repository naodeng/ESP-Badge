#include "travel_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put16(uint8_t *p, unsigned n){p[0]=n;p[1]=n>>8;}
static void frame(uint8_t p[20],unsigned seq){
    memset(p,0,20);p[0]=1;p[1]=7;put16(p+2,seq);p[4]=42;
    put16(p+8,1000);put16(p+10,9050);put16(p+12,120);put16(p+14,100);
}
int main(void){
    travel_data_t s={0};uint8_t p[20];frame(p,65535);
    assert(travel_data_status(&s,100)==TRAVEL_WAITING);
    assert(travel_data_accept(&s,p,20,100));
    assert(s.speed_cms==1000 && s.heading_cdeg==9050 && s.accuracy_dm==120);
    assert(travel_data_status(&s,5000)==TRAVEL_LIVE);
    assert(travel_data_status(&s,5001)==TRAVEL_STALE);
    assert(!travel_data_accept(&s,p,20,5100)); // duplicate must not refresh age
    assert(travel_data_status(&s,5100)==TRAVEL_STALE);
    frame(p,0);assert(travel_data_accept(&s,p,20,6000)); // sequence rollover
    frame(p,65535);assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);p[4]=43;assert(!travel_data_accept(&s,p,20,6001)); // same connection, different session
    frame(p,1);p[0]=2;assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);p[1]=128;assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);p[19]=1;assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);put16(p+10,36000);assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);put16(p+8,10001);assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);put16(p+14,5001);assert(!travel_data_accept(&s,p,20,6001));
    frame(p,1);assert(!travel_data_accept(&s,p,19,6001));assert(!travel_data_accept(&s,NULL,20,6001));
    assert(s.sequence==0 && s.received_ms==6000);
    frame(p,1);put16(p+8,149);assert(travel_data_accept(&s,p,20,6100));assert(!travel_data_heading_valid(&s));
    frame(p,2);put16(p+8,150);assert(travel_data_accept(&s,p,20,6200));assert(travel_data_heading_valid(&s));
    frame(p,3);p[1]=16;put16(p+8,65535);put16(p+10,65535);put16(p+12,65535);
    assert(travel_data_accept(&s,p,20,6300));assert(s.demo && !s.speed_valid && !travel_data_heading_valid(&s));
    frame(p,4);p[1]=0;assert(!travel_data_accept(&s,p,20,6400)); // missing flag needs sentinel
    travel_data_reset(&s);frame(p,0);assert(travel_data_accept(&s,p,20,UINT32_MAX-100));
    assert(travel_data_status(&s,100)==TRAVEL_LIVE);assert(travel_data_status(&s,5000)==TRAVEL_STALE);
    travel_data_reset(&s);assert(travel_data_status(&s,7000)==TRAVEL_WAITING);
    puts("travel: validation, atomic rejection, sequence/session, missing fields, low speed, expiry and clock wrap PASS");
}
