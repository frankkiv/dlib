============================Frank Noted================================= 


1. First You have  to install opencv

Install OpenCV 3 on Mac OSX with brew
reference  https://www.learnopencv.com/install-opencv-3-on-yosemite-osx-10-10-x/

You can now install OpenCV 3 using brew. See the next section to install from source. Life is good again!

brew tap homebrew/science
brew install opencv3
You can choose the different options you can use with install in the subsections below. Here is what I recommend

# Easy install for beginners
brew install opencv3 --with-contrib 
# For intermediate and advanced users. 
brew install opencv3 --with-contrib --with-cuda --with-ffmpeg --with-tbb --with-qt5

# If you already install the opencv version 2.4.xx
go to 
    example/CMakeList.txt:125
chage back to 
    find_package(OpenCV QUIET)

2. How to build

    cd example
    mkdir build
    cd build
    cmake ..

# On my OSX
#    cmake -DCMAKE_C_COMPILER=/usr/local/bin/gcc-5 -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-5 ..
# or cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
    
    vim CMakeCache.txt
    # Find   CMAKE_CXX_FLAGS:STRING
    # Add -fcilkplus -lcilkrts

    make 

3. How to run 

# Use default camera
    CILK_NWORKERS=1 ./webcam_face_pose_ex 

# Use video clip
    CILK_NWORKERS=1 ./webcam_face_pose_ex TestVideo.avi

# Issue
    CILK_NWORKERS= 2,3,4 get Error

dlib C++ library

Dlib is a modern C++ toolkit containing machine learning algorithms and tools
for creating complex software in C++ to solve real world problems.  See
http://dlib.net for the main project documentation and API reference.



COMPILING DLIB C++ EXAMPLE PROGRAMS
   Go into the examples folder and type:
       mkdir build; cd build; cmake .. ; cmake --build .
   That will build all the examples.  If you have a CPU that supports AVX
   instructions then turn them on like this:
       mkdir build; cd build; cmake .. -DUSE_AVX_INSTRUCTIONS=1; cmake --build .
   Doing so will make some things run faster.

COMPILING DLIB Python API
   Before you can run the Python example programs you must compile dlib. Type:
       python setup.py install
   or type
       python setup.py install --yes USE_AVX_INSTRUCTIONS
   if you have a CPU that supports AVX instructions, since this makes some
   things run faster.  

RUNNING THE UNIT TEST SUITE
   Type the following to compile and run the dlib unit test suite:
       cd dlib/test
       mkdir build
       cd build
       cmake ..
       cmake --build . --config Release
       ./dtest --runall

   Note that on windows your compiler might put the test executable in a
   subfolder called Release.  If that's the case then you have to go to that
   folder before running the test.

This library is licensed under the Boost Software License, which can be found
in dlib/LICENSE.txt.  The long and short of the license is that you can use
dlib however you like, even in closed source commercial software.

Dlib Sponsors:
  This research is based in part upon work supported by the Office of the
  Director of National Intelligence (ODNI), Intelligence Advanced Research
  Projects Activity (IARPA) under contract number 2014-14071600010. The
  views and conclusions contained herein are those of the authors and
  should not be interpreted as necessarily representing the official policies
  or endorsements, either expressed or implied, of ODNI, IARPA, or the U.S.
  Government.  

