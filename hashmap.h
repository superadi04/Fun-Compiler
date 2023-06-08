#pragma once

#include <stdnoreturn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hashmap.h"

#define MAP_SIZE 2

struct Pair {
    char* key;
    int value;
};

struct HashMap {
    struct Pair **variables;
    size_t CURR_SIZE;
    size_t SIZE;
};

size_t hash(const char *name) {
    size_t out = 5381;
    for (size_t i = 0; i < strlen(name); i++) {
        char const c = name[i];
        out = out * 33 + c;
    }
    return out;
}

// Initialize all values in functions to null
struct HashMap init_variable_table() {
    struct HashMap map;
    map.CURR_SIZE = 2;
    map.SIZE = 0;
    map.variables = malloc(sizeof(struct Pair*) * MAP_SIZE);

    for (int i = 0; i < MAP_SIZE; i++) {
        map.variables[i] = NULL;
    }

    return map;
}

struct HashMap resize_map(struct HashMap map) {
    size_t CURR_SIZE = map.CURR_SIZE;
    CURR_SIZE *= 2;

    struct Pair **resizeVariables = malloc(sizeof(struct Pair*) * CURR_SIZE);

    for (int i = 0; i < CURR_SIZE; i++) {
        resizeVariables[i] = NULL;
    }

    for (int i = 0; i < CURR_SIZE / 2; i++) {
        struct Pair *entry = map.variables[i];
        size_t index = hash(entry->key) % (CURR_SIZE);

        for (int j = index; j < index + CURR_SIZE; j++)
        {
            // Find first value that is null, or w/ same key
            if (resizeVariables[j % CURR_SIZE] == NULL)
            {
                resizeVariables[j % CURR_SIZE] = entry;
                break;
            }
        }
    }

    free(map.variables);

    struct HashMap resizedMap;
    resizedMap.CURR_SIZE = CURR_SIZE;
    resizedMap.variables = resizeVariables;
    resizedMap.SIZE = map.SIZE;

    return resizedMap;
}

// Check if 2 strings are equal
bool checkEqualStringFunction(char const *a, char *b, size_t len) {
    while (*b != 0 && *a != 0) {
        if (*a != *b) {
            return false;
        }
        a++;
        b++;
    }

    return *b == 0 && *a == 0;
}

void set_variable(char *key, int value, struct HashMap map)
{
    // Get index to place pair w/ modulus
    size_t index = hash(key) % map.CURR_SIZE;

    for (int i = index; i < index + map.CURR_SIZE; i++)
    {
        // Find first value that is null, or w/ same key
        if (map.variables[i % map.CURR_SIZE] != NULL && checkEqualStringFunction(key, map.variables[i % map.CURR_SIZE]->key, strlen(key)))
        {
            map.variables[i % map.CURR_SIZE]->value = value;

            return;
        }
    }
}

struct HashMap add_value(char *key, int value, struct HashMap map)
{
    // Get index to place pair w/ modulus
    size_t index = hash(key) % map.CURR_SIZE;

    struct Pair *item = malloc(sizeof(struct Pair));
    item->key = key;
    item->value = value;

    for (int i = index; i < index + map.CURR_SIZE; i++)
    {
        // Find first value that is null, or w/ same key
        if (map.variables[i % map.CURR_SIZE] == NULL || checkEqualStringFunction(key, (map.variables[i % map.CURR_SIZE]->key), strlen(key)))
        {
            if (map.variables[i % map.CURR_SIZE] == NULL) {
                map.SIZE++;
                map.variables[i % map.CURR_SIZE] = item;
            } 
            
            return map;
        }
    }

    map = resize_map(map);
    map = add_value(key, value, map);
    return map;
}


bool contains_key(const char *key, struct HashMap map)
{
    // Get index to place pair w/ modulus
    size_t index = hash(key) % map.CURR_SIZE;

    for (int i = index; i < index + map.CURR_SIZE; i++)
    {
        // Find first value that is not null at index and is equal to key
        if (map.variables[i % map.CURR_SIZE] != NULL && checkEqualStringFunction(key, map.variables[i % map.CURR_SIZE]->key, strlen(key)))
        {
            return true;
        }
    }

    // Default return value
    return false;
}


int get_value(const char *key, struct HashMap map)
{
    // Get index to place pair w/ modulus
    size_t index = hash(key) % map.CURR_SIZE;

    for (int i = index; i < index + map.CURR_SIZE; i++)
    {
        // Find first value that is not null at index and is equal to key
        if (map.variables[i % map.CURR_SIZE] != NULL && checkEqualStringFunction(key, map.variables[i % map.CURR_SIZE]->key, strlen(key)))
        {
            return map.variables[i % map.CURR_SIZE]->value;
        }
    }

    // Default return value
    return 0;
}
