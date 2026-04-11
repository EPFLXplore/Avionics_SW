/*
 * Operators.cpp
 *
 *  Created on: Feb 3, 2021
 *      Author: AV Team
 */

#include "../Inc/Operators.h"
#include "cmsis_os.h"

void * operator new(size_t size) {
    return pvPortMalloc(size);
}

void * operator new[](size_t size) {
    return pvPortMalloc(size);
}

void operator delete(void* ptr) {
    vPortFree(ptr);
}

void operator delete[](void* ptr) {
    vPortFree(ptr);
}
