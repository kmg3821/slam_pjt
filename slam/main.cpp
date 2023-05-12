
#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>

#include<opencv2/core/core.hpp>

#include <System.h>

using namespace std;

void LoadImages(const string &strImagePath, const string &strPathTimes,
                vector<string> &vstrImages, vector<double> &vTimeStamps);

int main(int argc, char **argv)
{  
    int fps = 25;
    float dT = 1.f/fps;
    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM3::System SLAM("~/ORB_SLAM3/Vocabulary/ORBvoc.txt","./cam_intrinsic.yaml",ORB_SLAM3::System::MONOCULAR, true);

    double t_track = 0.f;
    // Main loop
    cv::Mat im;
    int proccIm = 0;
    for(;;)
    {
        // Read image from file
        im = cv::imread(vstrImageFilenames[seq][ni],cv::IMREAD_UNCHANGED); //,CV_LOAD_IMAGE_UNCHANGED);
        double tframe = vTimestampsCam[seq][ni];

        if(im.empty())
        {
            cerr << endl << "Failed to load image at: "
                    <<  vstrImageFilenames[seq][ni] << endl;
            return 1;
        }

        const auto t1 = std::chrono::steady_clock::now();

        SLAM.TrackMonocular(im,tframe); // TODO change to monocular_inertial

        const auto t2 = std::chrono::steady_clock::now();

        double ttrack= std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();

        vTimesTrack[ni]=ttrack;

        // Wait to load the next frame
        double T=0;
        if(ni<nImages[seq]-1)
            T = vTimestampsCam[seq][ni+1]-tframe;
        else if(ni>0)
            T = tframe-vTimestampsCam[seq][ni-1];

        //std::cout << "T: " << T << std::endl;
        //std::cout << "ttrack: " << ttrack << std::endl;

        if(ttrack<T) {
            //std::cout << "usleep: " << (dT-ttrack) << std::endl;
            usleep((T-ttrack)*1e6); // 1e6
        }
    }

    // Stop all threads
    SLAM.Shutdown();

    // Save camera trajectory
    if (bFileName)
    {
        const string kf_file =  "kf_" + string(argv[argc-1]) + ".txt";
        const string f_file =  "f_" + string(argv[argc-1]) + ".txt";
        SLAM.SaveTrajectoryEuRoC(f_file);
        SLAM.SaveKeyFrameTrajectoryEuRoC(kf_file);
    }
    else
    {
        SLAM.SaveTrajectoryEuRoC("CameraTrajectory.txt");
        SLAM.SaveKeyFrameTrajectoryEuRoC("KeyFrameTrajectory.txt");
    }

    return 0;
}

void LoadImages(const string &strImagePath, const string &strPathTimes,
                vector<string> &vstrImages, vector<double> &vTimeStamps)
{
    ifstream fTimes;
    fTimes.open(strPathTimes.c_str());
    vTimeStamps.reserve(5000);
    vstrImages.reserve(5000);
    while(!fTimes.eof())
    {
        string s;
        getline(fTimes,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            vstrImages.push_back(strImagePath + "/" + ss.str() + ".png");
            double t;
            ss >> t;
            vTimeStamps.push_back(t*1e-9);

        }
    }
}
