/*
 * microros_allocators.h
 *
 *  Created on: Nov 29, 2025
 *      Author: pedro
 */

#ifndef INC_MICROROS_ALLOCATORS_H_
#define INC_MICROROS_ALLOCATORS_H_


#include <stddef.h>
#include <rcutils/allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

void * microros_allocate(size_t size, void * state);
void microros_deallocate(void * pointer, void * state);
void * microros_reallocate(void * pointer, size_t size, void * state);
void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state);

#ifdef __cplusplus
}
#endif

#endif // MICROROS_ALLOCATORS_H


