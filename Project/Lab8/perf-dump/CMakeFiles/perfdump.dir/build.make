# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.9

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/cmake/bin/cmake

# The command to remove a file.
RM = /opt/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tislam/Tools/perf-dump-perThread

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tislam/Tools/perf-dump-perThread/BUILD-comet

# Include any dependencies generated for this target.
include CMakeFiles/perfdump.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/perfdump.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/perfdump.dir/flags.make

CMakeFiles/perfdump.dir/perf_dump.C.o: CMakeFiles/perfdump.dir/flags.make
CMakeFiles/perfdump.dir/perf_dump.C.o: ../perf_dump.C
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tislam/Tools/perf-dump-perThread/BUILD-comet/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/perfdump.dir/perf_dump.C.o"
	/opt/mvapich2/intel/ib/bin/mpic++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/perfdump.dir/perf_dump.C.o -c /home/tislam/Tools/perf-dump-perThread/perf_dump.C

CMakeFiles/perfdump.dir/perf_dump.C.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/perfdump.dir/perf_dump.C.i"
	/opt/mvapich2/intel/ib/bin/mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tislam/Tools/perf-dump-perThread/perf_dump.C > CMakeFiles/perfdump.dir/perf_dump.C.i

CMakeFiles/perfdump.dir/perf_dump.C.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/perfdump.dir/perf_dump.C.s"
	/opt/mvapich2/intel/ib/bin/mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tislam/Tools/perf-dump-perThread/perf_dump.C -o CMakeFiles/perfdump.dir/perf_dump.C.s

CMakeFiles/perfdump.dir/perf_dump.C.o.requires:

.PHONY : CMakeFiles/perfdump.dir/perf_dump.C.o.requires

CMakeFiles/perfdump.dir/perf_dump.C.o.provides: CMakeFiles/perfdump.dir/perf_dump.C.o.requires
	$(MAKE) -f CMakeFiles/perfdump.dir/build.make CMakeFiles/perfdump.dir/perf_dump.C.o.provides.build
.PHONY : CMakeFiles/perfdump.dir/perf_dump.C.o.provides

CMakeFiles/perfdump.dir/perf_dump.C.o.provides.build: CMakeFiles/perfdump.dir/perf_dump.C.o


CMakeFiles/perfdump.dir/papi_utils.C.o: CMakeFiles/perfdump.dir/flags.make
CMakeFiles/perfdump.dir/papi_utils.C.o: ../papi_utils.C
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tislam/Tools/perf-dump-perThread/BUILD-comet/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/perfdump.dir/papi_utils.C.o"
	/opt/mvapich2/intel/ib/bin/mpic++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/perfdump.dir/papi_utils.C.o -c /home/tislam/Tools/perf-dump-perThread/papi_utils.C

CMakeFiles/perfdump.dir/papi_utils.C.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/perfdump.dir/papi_utils.C.i"
	/opt/mvapich2/intel/ib/bin/mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tislam/Tools/perf-dump-perThread/papi_utils.C > CMakeFiles/perfdump.dir/papi_utils.C.i

CMakeFiles/perfdump.dir/papi_utils.C.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/perfdump.dir/papi_utils.C.s"
	/opt/mvapich2/intel/ib/bin/mpic++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tislam/Tools/perf-dump-perThread/papi_utils.C -o CMakeFiles/perfdump.dir/papi_utils.C.s

CMakeFiles/perfdump.dir/papi_utils.C.o.requires:

.PHONY : CMakeFiles/perfdump.dir/papi_utils.C.o.requires

CMakeFiles/perfdump.dir/papi_utils.C.o.provides: CMakeFiles/perfdump.dir/papi_utils.C.o.requires
	$(MAKE) -f CMakeFiles/perfdump.dir/build.make CMakeFiles/perfdump.dir/papi_utils.C.o.provides.build
.PHONY : CMakeFiles/perfdump.dir/papi_utils.C.o.provides

CMakeFiles/perfdump.dir/papi_utils.C.o.provides.build: CMakeFiles/perfdump.dir/papi_utils.C.o


# Object files for target perfdump
perfdump_OBJECTS = \
"CMakeFiles/perfdump.dir/perf_dump.C.o" \
"CMakeFiles/perfdump.dir/papi_utils.C.o"

# External object files for target perfdump
perfdump_EXTERNAL_OBJECTS =

libperfdump.a: CMakeFiles/perfdump.dir/perf_dump.C.o
libperfdump.a: CMakeFiles/perfdump.dir/papi_utils.C.o
libperfdump.a: CMakeFiles/perfdump.dir/build.make
libperfdump.a: CMakeFiles/perfdump.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tislam/Tools/perf-dump-perThread/BUILD-comet/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libperfdump.a"
	$(CMAKE_COMMAND) -P CMakeFiles/perfdump.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/perfdump.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/perfdump.dir/build: libperfdump.a

.PHONY : CMakeFiles/perfdump.dir/build

CMakeFiles/perfdump.dir/requires: CMakeFiles/perfdump.dir/perf_dump.C.o.requires
CMakeFiles/perfdump.dir/requires: CMakeFiles/perfdump.dir/papi_utils.C.o.requires

.PHONY : CMakeFiles/perfdump.dir/requires

CMakeFiles/perfdump.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/perfdump.dir/cmake_clean.cmake
.PHONY : CMakeFiles/perfdump.dir/clean

CMakeFiles/perfdump.dir/depend:
	cd /home/tislam/Tools/perf-dump-perThread/BUILD-comet && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tislam/Tools/perf-dump-perThread /home/tislam/Tools/perf-dump-perThread /home/tislam/Tools/perf-dump-perThread/BUILD-comet /home/tislam/Tools/perf-dump-perThread/BUILD-comet /home/tislam/Tools/perf-dump-perThread/BUILD-comet/CMakeFiles/perfdump.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/perfdump.dir/depend

