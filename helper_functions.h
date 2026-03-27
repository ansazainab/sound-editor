#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

//segments in a track
struct segment {
    int16_t* buffer; //int array associated with the segment
    size_t length; //length of buffer/segment
    struct segment* parent; //link to its parent (if any)
    struct segment** children; //array of its children
    int8_t num_children; //number of children
    struct segment* next; //link to next segment in track
};

//a track
struct sound_seg {
    int16_t** original_buffers; //array of malloced buffer pointers so they can be freed at the end
    int8_t num_buffers; //number of original buffers
    size_t length; //length of the track
    struct segment* segments; //linked list of segments in the track
};

extern struct segment* split_segment_after(struct segment* seg, size_t offset);

extern struct segment* split_segment_middle(struct segment* seg, size_t start, size_t end);

extern struct segment* split_all_one(struct segment* seg, size_t offset);

extern struct segment* split_all_two(struct segment* seg, size_t start, size_t end);