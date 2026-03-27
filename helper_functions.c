#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "helper_functions.h"

//splits a segment into 2 segments at offset with the right half as a new segment and left half's fields updated
struct segment* split_segment_after(struct segment* seg, size_t offset) {
    struct segment* right = malloc(sizeof(struct segment));
    if (!right) {
        return NULL;
    }
    right->buffer = seg->buffer + offset;
    right->length = seg->length - offset;
    right->parent = seg->parent;
    right->children = seg->children;
    right->num_children = seg->num_children;
    right->next = seg->next;
    
    seg->length = offset;
    seg->next = right;
    return right;
}

//splits segment into 3 segments at start and end with middle & right parts as new segments, left part's fields updated
struct segment* split_segment_middle(struct segment* seg, size_t start, size_t end) {
    struct segment* middle = malloc(sizeof(struct segment));
    if (!middle) {
        return NULL;
    }
    middle->buffer = seg->buffer + start;
    middle->length = end - start;
    middle->parent = seg->parent;
    middle->children = seg->children;
    middle->num_children = seg->num_children;
    middle->next = NULL;
    
    struct segment* right = NULL;
    right = split_segment_after(seg, end);
    if (!right) {
        free(middle);
        return NULL;
    }
    middle->next = right;
    
    seg->length = start;
    seg->next = middle;
    return middle;
}

//recursive function to split into 2 parts all segments in a tree of parent-child relationships starting with the root
struct segment* split_all_one(struct segment* seg, size_t offset) {
    //creating new list to store new right children of new right segment, if any
    struct segment** new_children = NULL;
    if (seg->num_children > 0) {
        new_children = malloc(seg->num_children*sizeof(struct segment*));
        if (!new_children) {
            return NULL;
        }
    }
    
    //split recursively for each child and adding new parts to new list
    for (int i = 0; i < seg->num_children; i++) {
        struct segment* new_child = split_all_one(seg->children[i], offset);
        if (!new_child) {
            return NULL;
        }
        new_children[i] = new_child;
    }
    
    //split the current segment itself
    struct segment* right = split_segment_after(seg, offset);
    if (!right) {
        return NULL;
    }
    right->children = new_children;
    
    for (int i = 0; i < right->num_children; i++) { //set new parent for each new right child
        new_children[i]->parent = right;
    }
    return right;
}

//recursive function to split into 3 parts all segments in a tree of parent-child relationships starting with the root
struct segment* split_all_two(struct segment* seg, size_t start, size_t end) {
    //creating new list to store new middle and right children of new middle and right segments, if any
    struct segment** middle_children = NULL;
    struct segment** right_children = NULL;
    if (seg->num_children > 0) {
        middle_children = malloc(seg->num_children*sizeof(struct segment*));
        if (!middle_children) {
            return NULL;
        }
        right_children =  malloc(seg->num_children*sizeof(struct segment*));
        if (!right_children) {
            return NULL;
        }
    }
    
    //split recursively for each child and adding new parts to new lists
    for (int i = 0; i < seg->num_children; i++) {
        struct segment* middle_child = split_all_two(seg->children[i], start, end);
        if (!middle_child) {
            return NULL;
        }
        middle_children[i] = middle_child;
        right_children[i] = middle_child->next;
    }
    
    //split the current segment itself
    struct segment* middle = split_segment_middle(seg, start, end);
    if (!middle) {
        return NULL;
    }
    middle->children = middle_children;
    middle->next->children = right_children;
    
    for (int i = 0; i < middle->num_children; i++) { //set new parents for each new middle and left child
        middle_children[i]->parent = middle;
        right_children[i]->parent = middle->next;
    }
    return middle;
}