#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "helper_functions.h"

// Load a WAV file into buffer
void wav_load(const char* filename, int16_t* dest) {
    int header_length = 44;
    int datasize_length = 4;
    FILE* file = fopen(filename, "rb");
    fseek(file, header_length-datasize_length, SEEK_SET);
    uint32_t data_size = 0;
    fread(&data_size, sizeof(uint32_t), 1, file);
    fread(dest, sizeof(int16_t), data_size/sizeof(int16_t), file);
    fclose(file);
    return;
}

// Create/write a WAV file from buffer
void wav_save(const char* fname, int16_t* src, size_t len) {
    FILE* file = fopen(fname, "wb");
    
    //writing the RIFF chunk. all calculations according to formulas in wav website
    fwrite("RIFF", sizeof(char), 4, file);
    uint32_t subchunk2_size = len * sizeof(int16_t);
    uint32_t chunk_size = 36 + subchunk2_size;
    fwrite(&chunk_size, sizeof(uint32_t), 1, file);
    fwrite("WAVE", sizeof(char), 4, file);

    //writing the fmt chunk
    fwrite("fmt ", sizeof(char), 4, file);
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels = 1;
    uint32_t sample_rate = 8000;
    uint32_t byte_rate = sample_rate * num_channels * sizeof(int16_t);
    uint16_t block_align = num_channels * sizeof(int16_t);
    uint16_t bits_per_sample = 16;
    fwrite(&subchunk1_size, sizeof(uint32_t), 1, file);
    fwrite(&audio_format, sizeof(uint16_t), 1, file);
    fwrite(&num_channels, sizeof(uint16_t), 1, file);
    fwrite(&sample_rate, sizeof(uint32_t), 1, file);
    fwrite(&byte_rate, sizeof(uint32_t), 1, file);
    fwrite(&block_align, sizeof(uint16_t), 1, file);
    fwrite(&bits_per_sample, sizeof(uint16_t), 1, file);

    //writing the data chunk
    fwrite("data", sizeof(char), 4, file);
    fwrite(&subchunk2_size, sizeof(uint32_t), 1, file);
    fwrite(src, sizeof(int16_t), len, file);
    fclose(file);
    return;
}

// Initialize a new sound_seg object
struct sound_seg* tr_init() {
    struct sound_seg* track = malloc(sizeof(struct sound_seg));
    if (!track) {
        return NULL;
    }
    track->length = 0;
    track->segments = NULL;
    track->original_buffers = NULL;
    track->num_buffers = 0;
    return track;
}

// Destroy a sound_seg object and free all allocated memory
void tr_destroy(struct sound_seg* obj) {
    struct segment* seg = obj->segments;

    while (seg) {
        struct segment* temp = seg;
        seg = seg->next;
        temp->buffer = NULL;
        temp->parent = NULL;
        if (temp->children) {
            free(temp->children);
        }
        temp->children = NULL;
        temp->next = NULL;
        free(temp);
    }
    obj->segments = NULL;

    for (int i = 0; i < obj->num_buffers; i++) {
        free(obj->original_buffers[i]);
        obj->original_buffers[i] = NULL;
    }
    free(obj->original_buffers);
    obj->original_buffers = NULL;

    free(obj);
    obj = NULL;
    return;
}

// Return the length of the track
size_t tr_length(struct sound_seg* seg) {
    return seg->length;
}

// Read len elements from position pos into dest
void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len) {
    struct segment* seg = track->segments;
    size_t offset = 0;
    size_t copied = 0;
    
    //traverse through segments to find the one where pos is
    while (seg && copied < len) {
        //if pos in current segment, calculate how much to read and read from it.
        if (pos < offset + seg->length) {
            size_t start = 0;
            if (pos > offset) {
                start = pos - offset;
            }

            size_t available = seg->length - start;
            
            size_t to_copy = len - copied;
            if (available < (len - copied)) {
                to_copy = available;
            }

            memcpy(dest+copied, seg->buffer+start, to_copy*sizeof(int16_t));
            copied += to_copy;
            pos += to_copy;
        }

        offset += seg->length;
        seg = seg->next;
    }
    return;
}

// Write len elements from src into position pos
void tr_write(struct sound_seg* track, const int16_t* src, size_t pos, size_t len) {
    struct segment* seg = track->segments;
    struct segment* last = NULL;
    size_t offset = 0;
    size_t written = 0;
    
    //traverse through segments to find where pos is
    while (seg && written < len) {
        //if pos in current segment, calculate how much to write and write it in
        if (pos < offset + seg->length) {
            size_t start = 0;
            if (pos > offset) {
                start = pos - offset;
            }
            
            size_t available = seg->length - start;
            
            size_t to_write = len - written;
            if (available < (len - written)) {
                to_write = available;
            }
            
            memcpy(seg->buffer+start, src+written, to_write*sizeof(int16_t));
            written += to_write;
            pos += to_write;
        }
        
        offset += seg->length;
        last = seg;
        seg = seg->next;
    }
    
    //create a new segment if writing in track for the first time or extending its length
    if (written < len) {
        struct segment* new_seg = malloc(sizeof(struct segment));
        if (!new_seg) {
            return;
        }
        size_t to_write = len - written;
        new_seg->buffer = malloc(to_write*sizeof(int16_t));
        if (!new_seg->buffer) {
            free(new_seg);
            return;
        }
        
        //add to original buffers array since new buffer is being malloced
        int8_t new_count = track->num_buffers + 1;
        int16_t** new_buf = realloc(track->original_buffers, new_count*sizeof(int16_t*));
        if (!new_buf) {
            return;
        }
        track->original_buffers = new_buf;
        track->original_buffers[track->num_buffers] = new_seg->buffer;
        track->num_buffers = new_count;

        //write remaining length and set default values for new segment created
        memcpy(new_seg->buffer, src+written, to_write*sizeof(int16_t));
        new_seg->length = to_write;
        new_seg->parent = NULL;
        new_seg->children = NULL;
        new_seg->num_children = 0;
        new_seg->next = NULL;
        
        //adjust links of segments in the linked list
        if (last) {
            last->next = new_seg;
        }
        else {
            track->segments = new_seg;
        }
        
        track->length += to_write;
    }
    return;
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    size_t offset = 0;
    struct segment* seg = track->segments;
    
    //traverse through segments to check if any segments spanning pos to pos+len have children
    while (seg) {
        if (offset + seg->length > pos && offset < pos + len) {
            if (seg->num_children > 0) {
                return false;
            }
        }
        else if (offset > pos + len) {
            break;
        }
        offset += seg->length;
        seg = seg->next;
    }
    
    offset = 0;
    seg = track->segments;
    struct segment* prev = NULL; //to track last segment before deletion region begins
    bool prev_set = false;
    int prev_length = 0;
    struct segment* del_head = NULL; //head of deletion region
    struct segment* del_tail = NULL;
    size_t del_tail_length = 0;
    size_t remaining = len;
    
    //traverse through segments to find pos
    while (seg && remaining > 0) {
        //skip if not in pos to pos+len
        if (offset + seg->length <= pos) {
            offset += seg->length;
            if (!prev_set) {
                prev = seg;
                prev_length = prev->length;
            }
            seg = seg->next;
            continue;
        }
        
        size_t start = 0;
        if (pos > offset) {
            start = pos - offset;
        }
        
        size_t available = seg->length - start;
        
        size_t to_delete = remaining;
        if (available < remaining) {
            to_delete = available;
        }
        
        //separate portions to delete based on where they are in each segment
        struct segment* new_seg = NULL;
        if (start == 0 && to_delete == seg->length) { //entire segment is to be deleted
            new_seg = seg;
            prev_set = true;
        }
        else if (start == 0 && (start + to_delete < seg->length)) { //start of segment is to be deleted
            //find the root and split all segments in a tree
            struct segment* root = seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_one(root, to_delete);
            if (!temp) {
                return false;
            }
            
            new_seg = seg;
            if (prev) {
                if (prev_length != prev->length) { //if prev got split as well, adjust it to be second half
                    prev = prev->next;
                    prev_length = prev->length;
                }
            }
            prev_set = true;
        }
        else if (start > 0 && (start + to_delete == seg->length)) { //end of segment is to be deleted
            struct segment* root = seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_one(root, start);
            if (!temp) {
                return false;
            }

            new_seg = seg->next;
            prev = seg;
            prev_length = prev->length;
            prev_set = true;
        }
        else if (start > 0 && (start + to_delete < seg->length)) { //middle of segment is to be deleted
            struct segment* root = seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_two(root, start, start + to_delete);
            if (!temp) {
                return false;
            }
            
            new_seg = seg->next;
            prev = seg;
            prev_length = prev->length;
            prev_set = true;
        }
        
        //add to list of segments to be deleted
        if (!del_head) {
            del_head = new_seg;
            del_tail = new_seg;
            del_tail_length = del_tail->length;
        }
        else {
            if (del_tail_length != del_tail->length) { //if tail of list to be deleted got split, adjust it
                del_tail = del_tail->next;
                del_tail_length = del_tail->length;
            }
            del_tail->next = new_seg;
            del_tail = new_seg;
            del_tail_length = del_tail->length;
        }
        
        remaining -= to_delete;
        pos += to_delete;
        offset += seg->length;
        if (remaining > 0) {
            seg = seg->next;
        }
    }
    
    //adjust links of track to skip over deletion region
    if (prev) {
        prev->next = del_tail->next;
    }
    else {
        track->segments = del_tail->next;
    }
    
    del_tail->next = NULL;
    struct segment* current = del_head;
    
    //remove each segment in list to be deleted from its parent's children array
    while (current) {
        struct segment* c_parent = current->parent;
        if (c_parent) {
            int index = 0;
            for (int i = 0; i < c_parent->num_children; i++) {
                if (c_parent->children[i] == current) {
                    index = i;
                    break;
                }
            }
            
            for (int j = index; j < c_parent->num_children-1; j++) {
                c_parent->children[j] = c_parent->children[j+1];
            }
            c_parent->num_children--;
            
            if (c_parent->num_children > 0) {
                struct segment** new_arr = realloc(c_parent->children, c_parent->num_children*sizeof(struct segment*));
                if (!new_arr) {
                    return false;
                }
                c_parent->children = new_arr;
            }
            else {
                free(c_parent->children);
                c_parent->children = NULL;
            }
        }
        current = current->next;
    }
    
    current = del_head;
    while (current) {
        struct segment* c_next = current->next;
        free(current);
        current = c_next;
    }

    track->length -= len;
    return true;
}

//compute the correlation between two arrays of samples
double compute_correlation(const int16_t* x, const int16_t* y, size_t len) {
    double sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += x[i] * y[i];
    }
    return sum;
}

// Returns a string containing <start>,<end> ad pairs in target
char* tr_identify(struct sound_seg* target, struct sound_seg* ad) {
    //combine buffers of each segment in target and ad to make it one continuous buffer
    int16_t* combined_t = malloc(target->length*sizeof(int16_t));
    if (!combined_t) {
        return NULL;
    }
    struct segment* seg = target->segments;
    size_t offset = 0;
    while (seg) {
        memcpy(combined_t+offset, seg->buffer, seg->length*sizeof(int16_t));
        offset += seg->length;
        seg = seg->next;
    }

    int16_t* combined_a = malloc(ad->length*sizeof(int16_t));
    if (!combined_a) {
        return NULL;
    }
    seg = ad->segments;
    offset = 0;
    while (seg) {
        memcpy(combined_a+offset, seg->buffer, seg->length*sizeof(int16_t));
        offset += seg->length;
        seg = seg->next;
    }

    size_t tlen = target->length;
    size_t alen = ad->length;
    char* result = malloc(256); //temporary memory to store string, size can be changed
    if (!result) {
        return NULL;
    }
    
    result[0] = '\0';
    size_t rlen = 0;
    double ref_corr = compute_correlation(combined_a, combined_a, alen);
    
    //iterate through each sample to check match
    for (size_t i = 0; i <= tlen-alen; i++) {
        double cross_corr = compute_correlation(combined_t+i, combined_a, alen);
        double match = (cross_corr/ref_corr)*100;
        if (match < 95) {
            continue;
        }
        
        //if match, stored indices in temporary buffer, extended result size if needed and concatenate with result
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%zu,%zu\n", i, i+alen-1);
        size_t blen = strlen(buffer);
        
        if (rlen+blen >= 256) {
            char* temp = realloc(result, rlen+blen+1);
            if (!temp) {
                free(result);
                return NULL;
            }
            result = temp;
        }
        
        strcat(result, buffer);
        rlen += blen;
    }
    
    size_t length = strlen(result);
    if (length > 1 && result[length-1] == '\n') {
        result[length-1] = '\0';
    }

    free(combined_t);
    free(combined_a);
    return result;
}

//duplicate a parent segment to insert the duplicated segment somewhere as a child
struct segment* duplicate_segment(struct segment* seg) {
    struct segment* dup = malloc(sizeof(struct segment));
    if (!dup) {
        return NULL;
    }
    dup->buffer = seg->buffer;
    dup->length = seg->length;
    dup->parent = seg;
    dup->children = NULL;
    dup->num_children = 0;
    dup->next = NULL;
    return dup;
}

//add 'child' to array of children of 'parent'
void add_child(struct segment* parent, struct segment* child) {
    int8_t new_count = parent->num_children + 1;
    struct segment** new_children = realloc(parent->children, new_count*sizeof(struct segment*));
    if (!new_children) {
        return;
    }
    parent->children = new_children;
    parent->children[parent->num_children] = child;
    parent->num_children = new_count;
    return;
}

// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track, struct sound_seg* dest_track, size_t destpos, size_t srcpos, size_t len) {
    size_t remaining = len;
    size_t offset = 0;
    struct segment* src_seg = src_track->segments;
    struct segment* dups_head = NULL; //head of insertion region
    struct segment* dups_tail = NULL;
    size_t dups_tail_length = 0;
    
    //traverse through segments to find srcpos
    while (src_seg && remaining > 0) {
        //skip if not in srcpos to srcpos+len
        if (offset + src_seg->length <= srcpos) {
            offset += src_seg->length;
            src_seg = src_seg->next;
            continue;
        }

        size_t start = 0;
        if (srcpos > offset) {
            start = srcpos - offset;
        }
        
        size_t available = src_seg->length - start;
        
        size_t take = remaining; //to_insert
        if (available < remaining) {
            take = available;
        }
        
        //separate portions to insert based on where they are in each segment
        struct segment* new_seg = NULL;
        if (start == 0 && take == src_seg->length) { //entire segment is to be inserted
            new_seg = src_seg;
        }
        else if (start == 0 && (start + take < src_seg->length)) { //start of segment is to be inserted
            //find the root and split all segments in a tree
            struct segment* root = src_seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_one(root, take);
            if (!temp) {
                return;
            }
            new_seg = src_seg;
        }
        else if (start > 0 && (start + take == src_seg->length)) { //end of segment is to be deleted
            struct segment* root = src_seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_one(root, start);
            if (!temp) {
                return;
            }
            new_seg = src_seg->next;
        }
        else if (start > 0 && (start + take < src_seg->length)) { //middle of segment is to be deleted
            struct segment* root = src_seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_two(root, start, start + take);
            if (!temp) {
                return;
            }
            new_seg = src_seg->next;
        }

        //duplicate segment to be inserted, add it as a child of parent and add it to a list of segments to be inserted
        struct segment* dup = duplicate_segment(new_seg);
        if (!dup) {
            return;
        }
        
        add_child(new_seg, dup);
        
        if (!dups_head) {
            dups_head = dup;
            dups_tail = dup;
            dups_tail_length = dups_tail->length;
        }
        else {
            if (dups_tail_length != dups_tail->length) { //if tail of list to be inserted got split, adjust it
                dups_tail = dups_tail->next;
                dups_tail_length = dups_tail->length;
            }
            dups_tail->next = dup;
            dups_tail = dup;
            dups_tail_length = dups_tail->length;
        }

        remaining -= take;
        srcpos += take;
        offset += src_seg->length;
        if (remaining > 0) {
            src_seg = src_seg->next;
        }
    }
    
    //insert duplicated segments in relevant destpos
    offset = 0;
    struct segment* dest_seg = dest_track->segments;
    if (!dest_seg) {
        dest_track->segments = dups_head;
        dest_track->length += len;
        return;
    }

    struct segment* dest_prev = NULL;
    while (dest_seg && (offset + dest_seg->length <= destpos)) {
        offset += dest_seg->length;
        dest_prev = dest_seg;
        dest_seg = dest_seg->next;
    }
    
    if (dest_seg) {
        size_t local_offset = destpos - offset;
        if (local_offset == 0) {
            if (dest_prev) { //insert at boundary between 2 segments
                dest_prev->next = dups_head;
                dups_tail->next = dest_seg;
            }
            else { //insert at start of track
                dest_track->segments = dups_head;
                dups_tail->next = dest_seg;
            }
        }
        else if (local_offset < dest_seg->length) { //insert in middle of segment
            struct segment* root = dest_seg;
            while (root->parent) {
                root = root->parent;
            }
            struct segment* temp = split_all_one(root, local_offset);
            if (!temp) {
                return;
            }
            
            if (dups_tail->length != dups_tail_length) {
                dups_tail = dups_tail->next;
                dups_tail_length = dups_tail->length;
            }
            dups_tail->next = dest_seg->next;
            dest_seg->next = dups_head;
        }
    }
    else { //insert at end of track
        dest_prev->next = dups_head;
    }
    
    dest_track->length += len;
    return;
}