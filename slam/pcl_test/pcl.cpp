
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/common/time.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/passthrough.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/segmentation/extract_clusters.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <chrono>

using namespace std;
using namespace pcl;

int main(int argc, char **argv)
{
    PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
    visualization::PCLVisualizer::Ptr viewer(new visualization::PCLVisualizer("Occupanct Grid Map1"));
    io::loadPCDFile<PointXYZ>("test.pcd", *cloud);

    VoxelGrid<PointXYZ> voxel_grid;
    voxel_grid.setInputCloud(cloud);
    voxel_grid.setLeafSize(0.1, 0.1, 0.1);
    PointCloud<PointXYZ>::Ptr cloud_filtered(new PointCloud<PointXYZ>);
    voxel_grid.filter(*cloud_filtered);

    cout << cloud->size() << "," << cloud_filtered->size() << endl;

    // float res = 0.01;
    // int map_width = 100;
    // int map_height = 100;

    // vector<vector<bool>> om(map_height, vector<bool>(map_width, false));

    // for (const auto &point : cloud_filtered->points)
    // {
    //     int x = static_cast<int>(point.z / res) + map_width / 2;
    //     int y = static_cast<int>(point.x / res) + map_height / 2;

    //     if (x >= 0 && x < map_width && y >= 0 && y < map_height)
    //     {
    //         om[y][x] = 1;
    //     }
    // }

    // for (int i = 0; i < map_height; ++i)
    // {
    //     for (int j = 0; j < map_width; ++j)
    //     {
    //         cout << (om[i][j] ? "#" : ".");
    //     }
    //     cout << endl;
    // }

    viewer->addPointCloud<PointXYZ>(cloud_filtered, "cloud");
    viewer->setPointCloudRenderingProperties(visualization::PCL_VISUALIZER_POINT_SIZE, 1.5, "cloud");

    // float cell_size = 0.01;
    // float start_x = -map_width / 2.0 * cell_size;
    // float start_y = -map_height / 2.0 * cell_size;
    // for (int i = 0; i < map_height; ++i)
    // {
    //     for (int j = 0; j < map_width; ++j)
    //     {
    //         if (om[i][j] == 1)
    //         {
    //             viewer->addCube(start_x + j * cell_size, start_x + (j + 1) * cell_size,
    //                             start_y + j * cell_size, start_y + (j + 1) * cell_size,
    //                             0.0, 0.0, 1.0, 1.0, 1.0, to_string(i)+to_string(j));
    //         }
    //     }
    // }

    viewer->spin();

    return 0;
}
