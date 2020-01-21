#ifndef MAI_BASE_QUEUE_MACROS_H_
#define MAI_BASE_QUEUE_MACROS_H_


#define QUEUE_HEADER(type) \
    type *next_; \
    type *prev_

#define QUEUE_INSERT_HEAD(h, x) \
    (x)->next_ = (h)->next_; \
    (x)->next_->prev_ = x; \
    (x)->prev_ = h; \
    (h)->next_ = x

#define QUEUE_INSERT_TAIL(h, x) \
    (x)->prev_ = (h)->prev_; \
    (x)->prev_->next_ = x; \
    (x)->next_ = h; \
    (h)->prev_ = x \

#define QUEUE_REMOVE(x) \
    (x)->next_->prev_ = (x)->prev_; \
    (x)->prev_->next_ = (x)->next_; \
    (x)->prev_ = NULL; \
    (x)->next_ = NULL

#define QUEUE_EMPTY(h) (h)->next_ = (h)

#endif // MAI_BASE_QUEUE_MACROS_H_
