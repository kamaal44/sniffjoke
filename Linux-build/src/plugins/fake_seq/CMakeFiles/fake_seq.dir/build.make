# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.4

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/vecna/Desktop/sniffjoke-project/sniffjoke

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build

# Include any dependencies generated for this target.
include src/plugins/fake_seq/CMakeFiles/fake_seq.dir/depend.make

# Include the progress variables for this target.
include src/plugins/fake_seq/CMakeFiles/fake_seq.dir/progress.make

# Include the compile flags for this target's objects.
include src/plugins/fake_seq/CMakeFiles/fake_seq.dir/flags.make

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/depend.make.mark: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/flags.make
src/plugins/fake_seq/CMakeFiles/fake_seq.dir/depend.make.mark: ../src/plugins/fake_seq/fake_seq.cc

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/flags.make
src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o: ../src/plugins/fake_seq/fake_seq.cc
	$(CMAKE_COMMAND) -E cmake_progress_report /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o"
	/usr/bin/c++   $(CXX_FLAGS) -o src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o -c /home/vecna/Desktop/sniffjoke-project/sniffjoke/src/plugins/fake_seq/fake_seq.cc

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.i"
	/usr/bin/c++  $(CXX_FLAGS) -E /home/vecna/Desktop/sniffjoke-project/sniffjoke/src/plugins/fake_seq/fake_seq.cc > src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.i

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.s"
	/usr/bin/c++  $(CXX_FLAGS) -S /home/vecna/Desktop/sniffjoke-project/sniffjoke/src/plugins/fake_seq/fake_seq.cc -o src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.s

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o.requires:

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o.provides: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o.requires
	$(MAKE) -f src/plugins/fake_seq/CMakeFiles/fake_seq.dir/build.make src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o.provides.build

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o.provides.build: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/depend: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/depend.make.mark

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/depend.make.mark:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --magenta --bold "Scanning dependencies of target fake_seq"
	cd /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/vecna/Desktop/sniffjoke-project/sniffjoke /home/vecna/Desktop/sniffjoke-project/sniffjoke/src/plugins/fake_seq /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build/src/plugins/fake_seq /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build/src/plugins/fake_seq/CMakeFiles/fake_seq.dir/DependInfo.cmake

# Object files for target fake_seq
fake_seq_OBJECTS = \
"CMakeFiles/fake_seq.dir/fake_seq.o"

# External object files for target fake_seq
fake_seq_EXTERNAL_OBJECTS =

src/plugins/fake_seq/libfake_seq.so: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o
src/plugins/fake_seq/libfake_seq.so: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/build.make
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX shared library libfake_seq.so"
	cd /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build/src/plugins/fake_seq && $(CMAKE_COMMAND) -P CMakeFiles/fake_seq.dir/cmake_clean_target.cmake
	cd /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build/src/plugins/fake_seq && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fake_seq.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/plugins/fake_seq/CMakeFiles/fake_seq.dir/build: src/plugins/fake_seq/libfake_seq.so

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/requires: src/plugins/fake_seq/CMakeFiles/fake_seq.dir/fake_seq.o.requires

src/plugins/fake_seq/CMakeFiles/fake_seq.dir/clean:
	cd /home/vecna/Desktop/sniffjoke-project/sniffjoke/Linux-build/src/plugins/fake_seq && $(CMAKE_COMMAND) -P CMakeFiles/fake_seq.dir/cmake_clean.cmake
