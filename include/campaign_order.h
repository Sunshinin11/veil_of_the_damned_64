#ifndef CAMPAIGN_ORDER_H
#define CAMPAIGN_ORDER_H
// Stable save/art IDs: cemetery=0, forest=1, snow=2, desert=3, city=4.
static inline int campaign_order_at(int position) {
    static const unsigned char order[5]={2,0,3,1,4};
    return order[(position%5+5)%5];
}
static inline int campaign_order_position(int campaign) {
    for(int i=0;i<5;i++)if(campaign_order_at(i)==campaign)return i;
    return 0;
}
static inline int campaign_order_step(int campaign,int step) {
    return campaign_order_at(campaign_order_position(campaign)+step);
}
#endif
