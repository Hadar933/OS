# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

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
CMAKE_COMMAND = /usr/local/APP/jetbrains/clion/2020.2/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /usr/local/APP/jetbrains/clion/2020.2/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /cs/usr/shulik10/OS/OS/Exercises/ex3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/ex3.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ex3.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ex3.dir/flags.make

CMakeFiles/ex3.dir/MapReduceFramework.cpp.o: CMakeFiles/ex3.dir/flags.make
CMakeFiles/ex3.dir/MapReduceFramework.cpp.o: ../MapReduceFramework.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ex3.dir/MapReduceFramework.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ex3.dir/MapReduceFramework.cpp.o -c /cs/usr/shulik10/OS/OS/Exercises/ex3/MapReduceFramework.cpp

CMakeFiles/ex3.dir/MapReduceFramework.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ex3.dir/MapReduceFramework.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/shulik10/OS/OS/Exercises/ex3/MapReduceFramework.cpp > CMakeFiles/ex3.dir/MapReduceFramework.cpp.i

CMakeFiles/ex3.dir/MapReduceFramework.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ex3.dir/MapReduceFramework.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/shulik10/OS/OS/Exercises/ex3/MapReduceFramework.cpp -o CMakeFiles/ex3.dir/MapReduceFramework.cpp.s

CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.o: CMakeFiles/ex3.dir/flags.make
CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.o: ../Sample_Client/SampleClient.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.o -c /cs/usr/shulik10/OS/OS/Exercises/ex3/Sample_Client/SampleClient.cpp

CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/shulik10/OS/OS/Exercises/ex3/Sample_Client/SampleClient.cpp > CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.i

CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/shulik10/OS/OS/Exercises/ex3/Sample_Client/SampleClient.cpp -o CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.s

CMakeFiles/ex3.dir/Barrier/Barrier.cpp.o: CMakeFiles/ex3.dir/flags.make
CMakeFiles/ex3.dir/Barrier/Barrier.cpp.o: ../Barrier/Barrier.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/ex3.dir/Barrier/Barrier.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ex3.dir/Barrier/Barrier.cpp.o -c /cs/usr/shulik10/OS/OS/Exercises/ex3/Barrier/Barrier.cpp

CMakeFiles/ex3.dir/Barrier/Barrier.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ex3.dir/Barrier/Barrier.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/shulik10/OS/OS/Exercises/ex3/Barrier/Barrier.cpp > CMakeFiles/ex3.dir/Barrier/Barrier.cpp.i

CMakeFiles/ex3.dir/Barrier/Barrier.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ex3.dir/Barrier/Barrier.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/shulik10/OS/OS/Exercises/ex3/Barrier/Barrier.cpp -o CMakeFiles/ex3.dir/Barrier/Barrier.cpp.s

# Object files for target ex3
ex3_OBJECTS = \
"CMakeFiles/ex3.dir/MapReduceFramework.cpp.o" \
"CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.o" \
"CMakeFiles/ex3.dir/Barrier/Barrier.cpp.o"

# External object files for target ex3
ex3_EXTERNAL_OBJECTS =

ex3: CMakeFiles/ex3.dir/MapReduceFramework.cpp.o
ex3: CMakeFiles/ex3.dir/Sample_Client/SampleClient.cpp.o
ex3: CMakeFiles/ex3.dir/Barrier/Barrier.cpp.o
ex3: CMakeFiles/ex3.dir/build.make
ex3: CMakeFiles/ex3.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable ex3"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ex3.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ex3.dir/build: ex3

.PHONY : CMakeFiles/ex3.dir/build

CMakeFiles/ex3.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ex3.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ex3.dir/clean

CMakeFiles/ex3.dir/depend:
	cd /cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /cs/usr/shulik10/OS/OS/Exercises/ex3 /cs/usr/shulik10/OS/OS/Exercises/ex3 /cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug /cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug /cs/usr/shulik10/OS/OS/Exercises/ex3/cmake-build-debug/CMakeFiles/ex3.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ex3.dir/depend

