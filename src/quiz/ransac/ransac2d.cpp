/* \author Aaron Brown */
// Quiz on implementing simple RANSAC line fitting

#include <unordered_set>
#include "../../render/render.h"
#include "../../processPointClouds.h"
#include "../../processPointClouds.cpp" // using templates for processPointClouds so also include .cpp to help linker

pcl::PointCloud<pcl::PointXYZ>::Ptr CreateData() {
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
    // Add inliers
    float scatter = 0.6;
    for (int i = -5; i < 5; i++) {
        double rx = 2 * (((double) rand() / (RAND_MAX)) - 0.5);
        double ry = 2 * (((double) rand() / (RAND_MAX)) - 0.5);
        pcl::PointXYZ point;
        point.x = i + scatter * rx;
        point.y = i + scatter * ry;
        point.z = 0;

        cloud->points.push_back(point);
    }
    // Add outliers
    int numOutliers = 10;
    while (numOutliers--) {
        double rx = 2 * (((double) rand() / (RAND_MAX)) - 0.5);
        double ry = 2 * (((double) rand() / (RAND_MAX)) - 0.5);
        pcl::PointXYZ point;
        point.x = 5 * rx;
        point.y = 5 * ry;
        point.z = 0;

        cloud->points.push_back(point);

    }
    cloud->width = cloud->points.size();
    cloud->height = 1;

    return cloud;

}

pcl::PointCloud<pcl::PointXYZ>::Ptr CreateData3D() {
    ProcessPointClouds<pcl::PointXYZ> pointProcessor;

    // Needs to be run from repo root.
    const auto points = pointProcessor.loadPcd("src/sensors/data/pcd/simpleHighway.pcd");
    assert(points->points.size() >= 2);
    return points;
}


pcl::visualization::PCLVisualizer::Ptr initScene() {
    pcl::visualization::PCLVisualizer::Ptr viewer(new pcl::visualization::PCLVisualizer("2D Viewer"));
    viewer->setBackgroundColor(0.133, 0.133, 0.133);
    viewer->initCameraParameters();
    viewer->setCameraPosition(0, 0, 15, 0, 1, 0);
    viewer->addCoordinateSystem(1.0);
    return viewer;
}

#ifdef FOR_FUTURE_REFERENCE

std::unordered_set<int> Ransac(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud, int maxIterations, float distanceTol) {
    std::unordered_set<int> inliersResult;
    srand(time(0));

    const auto numPoints = cloud->points.size();
    assert(numPoints >= 2);

    // For max iterations
    for (auto i = 0; i < maxIterations; ++i) {

        // Randomly sample subset and fit line.
        // We're using a set here to ensure we never sample the same point twice.
        std::unordered_set<int> inliers;
        while (inliers.size() < 2) {
            inliers.insert(rand() % numPoints);
        }

        // Get both points.
        auto itr = inliers.begin();
        const auto& p1 = cloud->points[*itr];
        itr++;
        const auto& p2 = cloud->points[*itr];

        // Determine the line through both points.
        const auto a = p1.y - p2.y;
        const auto b = p2.x - p1.x;
        const auto c = p1.x*p2.y - p2.x*p1.y;
        const auto normalizer = 1.0 / std::sqrt(a * a + b * b);

        // Measure distance between every point and fitted line
        for (auto j = 0; j < numPoints; ++j) {

            // Skip points we already know are inliers.
            if (inliers.count(j) > 0) {
                continue;
            }

            // Determine the distance of the current point to the line.
            const auto& pt = cloud->points[j];
            const auto px = pt.x;
            const auto py = pt.y;
            const auto d = std::abs(a*px + b*py + c) * normalizer;

            // If distance is smaller than threshold count it as inlier
            if (d <= distanceTol) {
                inliers.insert(j);
            }
        }

        // Return indices of inliers from fitted line with most inliers.
        // Specifically, we only keep the result if we found a better answer than
        // the one we already have.
        if (inliers.size() > inliersResult.size()) {
            inliersResult = inliers;
        }
    }

    return inliersResult;
}

#endif

std::unordered_set<int> RansacPlane(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud, int maxIterations, float distanceTol) {
    std::unordered_set<int> inliersResult;
    srand(time(0));

    const auto numPoints = cloud->points.size();
    assert(numPoints >= 3); // ... and points are not collinear ...

    // For max iterations
    for (auto i = 0; i < maxIterations; ++i) {

        // Randomly sample subset and fit a plane.
        // We're using a set here to ensure we never sample the same point twice.
        std::unordered_set<int> inliers;
        while (inliers.size() < 3) {
            inliers.insert(rand() % numPoints);
        }

        // Get all points.
        auto itr = inliers.begin();
        const auto& p1 = cloud->points[*itr];
        itr++;
        const auto& p2 = cloud->points[*itr];
        itr++;
        const auto& p3 = cloud->points[*itr];

        // Determine the plane through three points.
        const auto v1 = Vect3{p2} - Vect3{p1};
        const auto v2 = Vect3{p3} - Vect3{p1};

        // Determine plane normal by taking the cross product.
        const auto normal = v1.cross(v2);
        const auto normalizer = 1.0 / normal.norm();

        // Plane coefficients
        const auto A = normal.x;
        const auto B = normal.y;
        const auto C = normal.z;
        const auto D = (A * p1.x + B * p1.y + C * p1.z);

        // Measure distance between every point and fitted line
        for (auto j = 0; j < numPoints; ++j) {

            // Skip points we already know are inliers.
            if (inliers.count(j) > 0) {
                continue;
            }

            // Determine the distance of the current point to the line.
            const auto& pt = cloud->points[j];
            const auto d = std::abs(A * pt.x + B * pt.y + C * pt.z + D) * normalizer;

            // If distance is smaller than threshold count it as inlier
            if (d <= distanceTol) {
                inliers.insert(j);
            }
        }

        // Return indices of inliers from fitted line with most inliers.
        // Specifically, we only keep the result if we found a better answer than
        // the one we already have.
        if (inliers.size() > inliersResult.size()) {
            inliersResult = inliers;
        }
    }

    return inliersResult;
}

int main() {

    // Create viewer
    pcl::visualization::PCLVisualizer::Ptr viewer = initScene();

    // Create data
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud = CreateData3D();  // use CreateData() for line fitting

    // Time segmentation process
    auto startTime = std::chrono::steady_clock::now();

    // Run RANSAC to find the line.
    const auto maxIterations = 100;
    const auto distanceTolerance = 0.2;
    std::unordered_set<int> inliers = RansacPlane(cloud, maxIterations, distanceTolerance);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "plane segmentation took " << elapsedTime.count() << " ms" << std::endl;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudInliers(new pcl::PointCloud<pcl::PointXYZ>());
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudOutliers(new pcl::PointCloud<pcl::PointXYZ>());

    for (int index = 0; index < cloud->points.size(); index++) {
        pcl::PointXYZ point = cloud->points[index];
        if (inliers.count(index))
            cloudInliers->points.push_back(point);
        else
            cloudOutliers->points.push_back(point);
    }


    // Render 2D point cloud with inliers and outliers
    if (inliers.size()) {
        renderPointCloud(viewer, cloudInliers, "inliers", Color{0.831, 0.812, 0.788});
        renderPointCloud(viewer, cloudOutliers, "outliers", Color{0.831, 0.241, 0.312});
    } else {
        renderPointCloud(viewer, cloud, "data");
    }

    while (!viewer->wasStopped()) {
        viewer->spinOnce();
    }
}
