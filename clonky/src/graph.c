//
// reviewed: 2025-02-24
//
#include "graph.h"
#include "dc.h"
#include <stdio.h>
#include <stdlib.h>

struct graph {
    uint64_t *values;
    uint32_t nvalues;
    uint32_t ix;
};

/*gives*/ struct graph *graph_new(const uint32_t nvalues) {
    struct graph *self = calloc(1, sizeof(struct graph));
    if (!self) {
        puts("!!! cannot alloc struct graph_new");
        exit(1);
    }
    self->nvalues = nvalues;
    self->values = (uint64_t *)calloc(nvalues, sizeof(uint64_t));
    if (!self->values) {
        printf("!!! graph_new cannot alloc array of size %d\n", nvalues);
        exit(1);
    }
    self->ix = 0;
    return self;
}

void graph_del(/*takes*/ struct graph *self) {
    free(self->values);
    free(self);
}

void graph_add_value(struct graph *self, const uint64_t value) {
    self->values[self->ix] = value;
    self->ix++;
    if (self->ix == self->nvalues) {
        self->ix = 0;
    }
}

static uint64_t cap_value(const uint64_t value, const uint32_t height) {
    if (value > height) {
        return height;
    }
    return value;
}

void graph_draw(const struct graph *self, struct dc *dc, const uint32_t height,
                const uint64_t max_value) {
    const int32_t dc_x = dc_get_x(dc);
    const int32_t dc_y = dc_get_y(dc);
    // draw top line
    const int32_t top_y = dc_y - (int32_t)height;
    dc_draw_line(dc, dc_x, top_y, dc_x + (int32_t)self->nvalues - 1, top_y);
    // note: -1 because draw line includes the end point
    int32_t x = dc_x;
    // circular buffer, draw from current index to end
    for (uint32_t i = self->ix; i < self->nvalues; i++) {
        const uint64_t value =
            cap_value(self->values[i] * height / max_value, height);
        dc_draw_line(dc, x, dc_y - (int32_t)value, x, dc_y);
        x++;
    }
    // draw from 0 to current index
    for (uint32_t i = 0; i < self->ix; i++) {
        const uint64_t value =
            cap_value(self->values[i] * height / max_value, height);
        dc_draw_line(dc, x, dc_y - (int32_t)value, x, dc_y);
        x++;
    }
}
