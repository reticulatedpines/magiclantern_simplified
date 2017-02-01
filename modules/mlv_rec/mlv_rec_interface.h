
#ifndef __MLV_REC_INTERFACE_H__
#define __MLV_REC_INTERFACE_H__

/* interface to register callbacks for variouos stuff */
typedef void (*event_cbr_t) (uint32_t event, void *ctx, mlv_hdr_t *hdr);

typedef struct
{
    uint32_t event;
    void *ctx;
    event_cbr_t cbr;
} cbr_entry_t;


/* event types, can be combined */
#define MLV_REC_EVENT_STARTING    (1U<<0)
#define MLV_REC_EVENT_STARTED     (1U<<1)
#define MLV_REC_EVENT_STOPPING    (1U<<2)
#define MLV_REC_EVENT_STOPPED     (1U<<3)
#define MLV_REC_EVENT_CYCLIC      (1U<<4)
#define MLV_REC_EVENT_BLOCK       (1U<<5)

#if defined(__MLV_REC_C__)
uint32_t mlv_rec_unregister_cbr(event_cbr_t cbr);
uint32_t mlv_rec_register_cbr(uint32_t event, event_cbr_t cbr, void *ctx);
uint32_t mlv_rec_queue_block(mlv_hdr_t *hdr);
#else
extern WEAK_FUNC(ret_0) uint32_t mlv_rec_unregister_cbr(event_cbr_t cbr);
extern WEAK_FUNC(ret_0) uint32_t mlv_rec_register_cbr(uint32_t event, event_cbr_t cbr, void *ctx);
extern WEAK_FUNC(ret_0) uint32_t mlv_rec_queue_block(mlv_hdr_t *hdr);
#endif

#endif
