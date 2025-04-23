#
# Copyright (C) 2024 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

if(CONFIG_SYSTEM_WINDOW_SERVICE)
  set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
  set(CUR_TARGET window_manager)
  nuttx_add_library(${CUR_TARGET} STATIC)
  set(CUR_DEPENDS android_binder lvgl framework_utils)
  nuttx_add_dependencies(TARGET ${CUR_TARGET} DEPENDS ${CUR_DEPENDS})

  set(INCDIR ${CURRENT_DIR}/../include ${CURRENT_DIR}/../am/include
             ${CURRENT_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/aidl)

  file(GLOB_RECURSE CRCS ${CURRENT_DIR}/server/*.c)
  file(GLOB_RECURSE CXXRCS ${CURRENT_DIR}/common/*.cpp ${CURRENT_DIR}/app/*.cpp
       ${CURRENT_DIR}/server/*.cpp)

  # complie options
  target_compile_options(${CUR_TARGET} PRIVATE ${CFLAGS})
  target_sources(${CUR_TARGET} PRIVATE ${CRCS} ${CXXRCS})
  target_include_directories(${CUR_TARGET} PRIVATE ${INCDIR})

  # export header file
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_CXX_INCLUDE_DIRECTORIES ${CURRENT_DIR}/include
             ${CURRENT_DIR}/../include ${CMAKE_CURRENT_BINARY_DIR}/aidl)

  # aidl sources
  file(GLOB_RECURSE AIDLS aidl/*.aidl)
  nuttx_add_aidl(
    TARGET
    ${CUR_TARGET}
    AIDL_BASE_DIR
    ${CURRENT_DIR}/aidl
    AIDL_INCLUDE_DIR
    ${CURRENT_DIR}/aidl
    AIDL_HEADER_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/aidl
    AIDL_OUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/aidl
    AIDL_FLAGS
    --lang=cpp
    AIDLS
    ${AIDLS})

  # for window testcase
  macro(add_wm_testcase PROG MAIN)
    nuttx_add_application(
      MODULE
      ${CONFIG_SYSTEM_WINDOW_SERVICE_TEST}
      NAME
      ${PROG}
      STACKSIZE
      ${CONFIG_DEFAULT_TASK_STACKSIZE}
      SRCS
      ${MAIN}
      COMPILE_FLAGS
      ${FLAGS}
      DEPENDS
      ${CUR_TARGET}
      googletest)
  endmacro()

  if(CONFIG_SYSTEM_WINDOW_SERVICE_TEST)
    add_wm_testcase(BufferQueueTest test/BufferQueueTest.cpp)
    add_wm_testcase(InputChannelTest test/InputChannelTest.cpp)
    add_wm_testcase(InputMonitorTest test/InputMonitorTest.cpp)
    add_wm_testcase(IWindowManagerTest test/IWindowManagerTest.cpp)
    add_wm_testcase(lvgltest_attribute test/lvgltest_attribute.c)
  endif()

endif()
