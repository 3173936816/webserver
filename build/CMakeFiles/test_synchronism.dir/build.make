# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/lcy/projectsMySelf/MyWebserver

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lcy/projectsMySelf/MyWebserver/build

# Include any dependencies generated for this target.
include CMakeFiles/test_synchronism.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_synchronism.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_synchronism.dir/flags.make

CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.o: CMakeFiles/test_synchronism.dir/flags.make
CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.o: ../tests/test_synchronism.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lcy/projectsMySelf/MyWebserver/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.o"
	/opt/rh/devtoolset-11/root/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.o -c /home/lcy/projectsMySelf/MyWebserver/tests/test_synchronism.cc

CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.i"
	/opt/rh/devtoolset-11/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lcy/projectsMySelf/MyWebserver/tests/test_synchronism.cc > CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.i

CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.s"
	/opt/rh/devtoolset-11/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lcy/projectsMySelf/MyWebserver/tests/test_synchronism.cc -o CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.s

# Object files for target test_synchronism
test_synchronism_OBJECTS = \
"CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.o"

# External object files for target test_synchronism
test_synchronism_EXTERNAL_OBJECTS =

../bin/test_synchronism: CMakeFiles/test_synchronism.dir/tests/test_synchronism.cc.o
../bin/test_synchronism: CMakeFiles/test_synchronism.dir/build.make
../bin/test_synchronism: ../lib/libWebServer.so
../bin/test_synchronism: CMakeFiles/test_synchronism.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lcy/projectsMySelf/MyWebserver/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_synchronism"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_synchronism.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_synchronism.dir/build: ../bin/test_synchronism

.PHONY : CMakeFiles/test_synchronism.dir/build

CMakeFiles/test_synchronism.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_synchronism.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_synchronism.dir/clean

CMakeFiles/test_synchronism.dir/depend:
	cd /home/lcy/projectsMySelf/MyWebserver/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lcy/projectsMySelf/MyWebserver /home/lcy/projectsMySelf/MyWebserver /home/lcy/projectsMySelf/MyWebserver/build /home/lcy/projectsMySelf/MyWebserver/build /home/lcy/projectsMySelf/MyWebserver/build/CMakeFiles/test_synchronism.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_synchronism.dir/depend

