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

#include "hashmap.h"

#define MAP_SIZE 2

size_t FUNCTION_CURR_SIZE = 2;

struct Function
{
    char **params;
    size_t numParams;
};

struct FunctionPair
{
    char *key;
    struct Function *value;
};

size_t func_hash(const char *name) {
    size_t out = 5381;
    for (size_t i = 0; i < strlen(name); i++) {
        char const c = name[i];
        out = out * 33 + c;
    }
    return out;
}

struct FunctionPair** functions;

struct FunctionPair** resize_func_map() {
    FUNCTION_CURR_SIZE *= 2;

    struct FunctionPair **resizeFunctions = malloc(sizeof(struct FunctionPair*) * FUNCTION_CURR_SIZE);

    for (int i = 0; i < FUNCTION_CURR_SIZE; i++) {
        resizeFunctions[i] = NULL;
    }

    for (int i = 0; i < FUNCTION_CURR_SIZE / 2; i++) {
        struct FunctionPair *entry = functions[i];
        size_t index = func_hash(entry->key) % (FUNCTION_CURR_SIZE);

        for (int j = index; j < index + FUNCTION_CURR_SIZE; j++)
        {
            // Find first value that is null, or w/ same key
            if (resizeFunctions[j % FUNCTION_CURR_SIZE] == NULL)
            {
                resizeFunctions[j % FUNCTION_CURR_SIZE] = entry;
                break;
            }
        }
    }

    free(functions);
    return resizeFunctions;
}

// Initialize all values in functions to null
void init_function_table() {
    functions = malloc(sizeof(struct FunctionPair*) * MAP_SIZE);

    for (int i = 0; i < MAP_SIZE; i++) {
        functions[i] = NULL;
    }
}

void insert_function(char *key, struct Function *value)
{
    // Get index to place pair w/ modulus
    size_t index = func_hash(key) % FUNCTION_CURR_SIZE;

    struct FunctionPair *item = malloc(sizeof(struct FunctionPair));
    item->key = key;
    item->value = value;

    for (int i = index; i < index + FUNCTION_CURR_SIZE; i++)
    {
        // Find first value that is null, or w/ same key
        if (functions[i % FUNCTION_CURR_SIZE] == NULL || checkEqualStringFunction(key, (functions[i % FUNCTION_CURR_SIZE]->key), strlen(key)))
        {
            functions[i % FUNCTION_CURR_SIZE] = item;
            return;
        }
    }

    functions = resize_func_map();
    insert_function(key, value);
}

struct Function *get_function(const char *key)
{
    // Get index to place pair w/ modulus
    size_t index = func_hash(key) % FUNCTION_CURR_SIZE;

    for (int i = index; i < index + FUNCTION_CURR_SIZE; i++)
    {
        // Find first value that is not null at index and is equal to key
        if (functions[i % FUNCTION_CURR_SIZE] != NULL && checkEqualStringFunction(key, functions[i % FUNCTION_CURR_SIZE]->key, strlen(key)))
        {
            return functions[i % FUNCTION_CURR_SIZE]->value;
        }
    }

    // Default return value
    return NULL;
}

bool contains_function(const char *key)
{
    // Get index to place pair w/ modulus
    size_t index = func_hash(key) % FUNCTION_CURR_SIZE;

    for (int i = index; i < index + FUNCTION_CURR_SIZE; i++)
    {
        // Find first value that is not null at index and is equal to key
        if (functions[i % FUNCTION_CURR_SIZE] != NULL && checkEqualStringFunction(key, functions[i % FUNCTION_CURR_SIZE]->key, strlen(functions[i % FUNCTION_CURR_SIZE]->key)))
        {
            return true;
        }
    }

    // Default return value
    return false;
}
