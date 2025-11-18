/*
 * System.hpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */

#ifndef CORE_INC_SYSTEM_H_
#define CORE_INC_SYSTEM_H_

#include "TestTask.h"
#include "MassThread.h"


class System {
public:
	static void init();

	static TestTask* test;
	static MassThread* mass;
};

#endif /* CORE_INC_SYSTEM_H_ */
