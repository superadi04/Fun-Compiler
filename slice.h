#pragma once

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

struct Slice
{
  char const *start; // Pointer to start of String in heap
  size_t len; // Length of String
};

// First constructor
struct Slice new_slice1(char const *const start, size_t const len)
{
  struct Slice _slice = {start, len};
  return _slice;
}

// Second constructor
struct Slice new_slice2(char const *const start, char const *const end)
{
  struct Slice _slice = {start, (size_t)(end - start)};
  return _slice;
}

// Identify a slice
bool is_identifier(struct Slice _slice)
{
  char const *start = _slice.start;
  size_t len = _slice.len;

  if (len == 0)
    return 0;
  if (!isalpha(start[0]))
    return false;
  for (size_t i = 1; i < len; i++)
    if (!isalnum(start[i]))
      return false;
  return true;
}

// Print contents of slice struct in readable format
void print_slice(struct Slice _slice)
{
  char const *const start = _slice.start;
  size_t const len = _slice.len;

  for (size_t i = 0; i < len; i++)
  {
    printf("%c", start[i]);
  }
}

// Equals function for a Slice struct and character string
const inline bool operator1(char const *p, struct Slice _slice)
{
  char const *const start = _slice.start;
  size_t const len = _slice.len;

  for (size_t i = 0; i < len; i++)
  {
    if (p[i] != start[i])
      return false;
  }
  return p[len] == 0;
}

// Equals function for 2 Slice structs
const bool operator2(const struct Slice other1, const struct Slice other2)
{
  char const *const start1 = other1.start;
  size_t const len1 = other1.len;

  if (len1 != other2.len)
    return false;
  for (size_t i = 0; i < len1; i++)
  {
    if (other2.start[i] != start1[i])
      return false;
  }
  return true;
}

// Hash function for Slice struct
size_t hash_function(struct Slice key)
{
  size_t out = 5381;
  for (size_t i = 0; i < key.len; i++)
  {
    char const c = key.start[i];
    out = out * 33 + c;
  }
  return out;
}
