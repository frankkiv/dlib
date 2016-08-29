// The contents of this file are in the public domain. See LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*

    This example program shows how to find frontal human faces in an image and
    estimate their pose.  The pose takes the form of 68 landmarks.  These are
    points on the face such as the corners of the mouth, along the eyebrows, on
    the eyes, and so forth.  
    

    This example is essentially just a version of the face_landmark_detection_ex.cpp
    example modified to use OpenCV's VideoCapture object to read from a camera instead 
    of files.


    Finally, note that the face detector is fastest when compiled with at least
    SSE2 instructions enabled.  So if you are using a PC with an Intel or AMD
    chip then you should enable at least SSE2 instructions.  If you are using
    cmake to compile this program you can enable them by using one of the
    following commands when you create the build project:
        cmake path_to_dlib_root/examples -DUSE_SSE2_INSTRUCTIONS=ON
        cmake path_to_dlib_root/examples -DUSE_SSE4_INSTRUCTIONS=ON
        cmake path_to_dlib_root/examples -DUSE_AVX_INSTRUCTIONS=ON
    This will set the appropriate compiler options for GCC, clang, Visual
    Studio, or the Intel compiler.  If you are using another compiler then you
    need to consult your compiler's manual to determine how to enable these
    instructions.  Note that AVX is the fastest but requires a CPU from at least
    2011.  SSE4 is the next fastest and is supported by most current machines.  
*/

#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <time.h>
#include <sys/time.h>

using namespace dlib;
using namespace std;

#define FACE_DOWNSAMPLE_RATIO 1
#define SKIP_FRAMES 1
typedef unsigned long long timestamp_t;

float fpsUpdate(float dt)  
{  
    static float _total_frames = 0.0f;  
    static float _total_time = 0.0f;  
    ++_total_frames;  
    _total_time += dt/1000000;  
    float fps = _total_frames/_total_time;

    return fps;
}

int CLOCK()
{
    return clock();
}

// dlib rectangle to opencv rectangle with limit boundries
static cv::Rect dlibRectangleToOpenCV(int width, int height, dlib::rectangle r)
{

    cout << r.left() << endl;
    cout << r.bottom() << endl;
    cout << r.top() << endl;
    cout << r.right() << endl;

    if(r.left()<0){
        r.left() = 0;
    }else if (r.left()>width-1){
        r.left() = width-1;
    }
    if(r.bottom()<0){
        r.bottom() =0;
    }else if(r.bottom()>height-1){
        r.bottom() = height-1;
    }
    if(r.top()<0){
        r.top() =0;
    }else if(r.top()>height-1){
        r.top() = height-1;
    }
    if(r.right()<0){
        r.right() = 0;
    }else if (r.right()>width-1){
        r.right() = width-1;
    }

    return cv::Rect(cv::Point2i(r.left(), r.top()), cv::Point2i(r.right(), r.bottom()));
}

void draw_polyline(cv::Mat &img, const dlib::full_object_detection& d, const int start, const int end, bool isClosed = false)
{
    std::vector <cv::Point> points;
    for (int i = start; i <= end; ++i)
    {
        points.push_back(cv::Point(d.part(i).x(), d.part(i).y()));
    }
    cv::polylines(img, points, isClosed, cv::Scalar(255,0,0), 2, 16, 0);
     
}
 
void render_face(cv::Mat &img, const dlib::full_object_detection& d)
{
    DLIB_CASSERT
    (
     d.num_parts() == 68,
     "\n\t Invalid inputs were given to this function. "
     << "\n\t d.num_parts():  " << d.num_parts()
     );
     
    draw_polyline(img, d, 0, 16);           // Jaw line
    draw_polyline(img, d, 17, 21);          // Left eyebrow
    draw_polyline(img, d, 22, 26);          // Right eyebrow
    draw_polyline(img, d, 27, 30);          // Nose bridge
    draw_polyline(img, d, 30, 35, true);    // Lower nose
    draw_polyline(img, d, 36, 41, true);    // Left eye
    draw_polyline(img, d, 42, 47, true);    // Right Eye
    draw_polyline(img, d, 48, 59, true);    // Outer lip
    draw_polyline(img, d, 60, 67, true);    // Inner lip
}

int main(int argc, char *argv[])
{
    try
    {
        cv::VideoCapture cap;

        int frame_Width, frame_Height;

        if(argc > 1)
        {
            cap.open(string(argv[1]));
        }
        else
        {
            cap.open(0);
        }
        cap.set(cv::CAP_PROP_BUFFERSIZE, 3);
        frame_Width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
        frame_Height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

        // // Print source framerate 
        // double fps = cap.get(cv::CAP_PROP_FPS);
        // cout << "Frames per second using video.get(CV_CAP_PROP_FPS) : " << fps << endl;
        
        if (!cap.isOpened())
        {
            cerr << "Unable to connect to camera" << endl;
            return 1;
        }

        // fps counter begin
        int counter = 0;
        int frame_ct = 0;
        struct timeval t1, t2;
        // fps counter end

        // Load face detection and pose estimation models.
        frontal_face_detector detector = get_frontal_face_detector();
        shape_predictor pose_model;
        deserialize("../shape_predictor_68_face_landmarks.dat") >> pose_model;

        // Grab and process frames until the main window is closed by the user.
        for(;;)
        {
            // fps counter begin
            gettimeofday(&t1,0);
            // fps counter end

            // Grab a frame
            cv::Mat im;
            cv::Mat im_small, im_display, im_gray;

            if (!cap.read(im))             
                break;
            cap >> im;
            cv::resize(im, im_small, cv::Size(), 1.0/FACE_DOWNSAMPLE_RATIO, 1.0/FACE_DOWNSAMPLE_RATIO);
            
            // Turn OpenCV's Mat into something dlib can deal with.  Note that this just
            // wraps the Mat object, it doesn't copy anything.  So cimg is only valid as
            // long as temp is valid.  Also don't do anything to temp that would cause it
            // to reallocate the memory which stores the image as that will make cimg
            // contain dangling pointers.  This basically means you shouldn't modify temp
            // while using cimg.
            cv_image<bgr_pixel> cimg_small(im_small);
            cv_image<bgr_pixel> cimg(im);
            cvtColor(im_small, im_gray, CV_BGR2GRAY);
            cv_image<unsigned char> cimg_gray(im_gray);

            // Detect faces 
            // std::vector<rectangle> faces = detector(cimg);
            // Detect faces on resize image
            std::vector<rectangle> faces;
            if ( frame_ct % SKIP_FRAMES == 0 )
            {
                faces = detector(cimg_gray);
                
                // Find the pose of each face.
                std::vector<full_object_detection> shapes;

                for (unsigned long i = 0; i < faces.size(); ++i)
                {
                    cout << endl <<" DEBUG: faces= "<< endl;

                    // Resize obtained rectangle for full resolution image. 
                    rectangle r(
                       (long)(faces[i].left() * FACE_DOWNSAMPLE_RATIO),
                       (long)(faces[i].top() * FACE_DOWNSAMPLE_RATIO),
                       (long)(faces[i].right() * FACE_DOWNSAMPLE_RATIO),
                       (long)(faces[i].bottom() * FACE_DOWNSAMPLE_RATIO)
                    );
                    // Save face crop
                    cv::Mat crop = im(dlibRectangleToOpenCV(frame_Width, frame_Height, r));
                    //cv::imwrite("../faces/Test/crop"+std::to_string(frame_ct)+"_"+std::to_string(i)+".jpg", crop);

                    shapes.push_back(pose_model(cimg, r));
                    render_face(im, pose_model(cimg, r));

                    cv::rectangle(im, dlibRectangleToOpenCV(frame_Width, frame_Height, r), cvScalar(255,255,255), 1, cv::LINE_8, 0);
                }

            }
            // Image save 
            // cv::imwrite("../faces/Test/frame"+std::to_string(frame_ct)+".jpg", im);
            
            //Comment cuz opencv3.1 bug https://github.com/opencv/opencv/issues/5874
            //cv::imshow((cv::String)"Face Detect Render", im);
            
            // fps counter begin
            frame_ct++;
            gettimeofday(&t2,0);
            float dur = (((t2.tv_sec - t1.tv_sec)*1000000L+t2.tv_usec) - t1.tv_usec);
            printf("frame processing time %f s. avg fps %f. frame no %d. \n",dur/1000000, fpsUpdate(dur), frame_ct);
            // fps counter end

            //Comment cuz opencv3.1 bug https://github.com/opencv/opencv/issues/5874
            //if(cv::waitKey(1)==27)
            //    exit(0);
    
        }
        cap.release();
        cv::destroyWindow("Face Detect Render");
        return 0;

    }
    catch(serialization_error& e)
    {
        cout << "You need dlib's default face landmarking model file to run this example." << endl;
        cout << "You can get it from the following URL: " << endl;
        cout << "   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << endl;
        cout << endl << e.what() << endl;
    }
    catch(exception& e)
    {
        cout << e.what() << endl;
    }
}



