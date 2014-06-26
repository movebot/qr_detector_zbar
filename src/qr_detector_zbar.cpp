#include "qr_detector_zbar/qr_detector_zbar.h"

#include <zbar.h>
#include <opencv/highgui.h>

namespace qr_detector_zbar
{

bool getPoseFromCornerPoints(const cv::Mat& depth_image, const geo::DepthCamera& rasterizer, const std::vector<cv::Point2i>& pts, geo::Pose3D& pose, unsigned int scale)
{
    std::vector<geo::Vector3> v;

    get3DCornerPoints(depth_image,rasterizer,pts,v,scale);

    // Calculate the rotation
    geo::Vector3 b3 = (v[1]-v[0]).cross(v[3]-v[0]).normalize();
    geo::Vector3 b1 = (v[3]-v[0]).cross(b3).normalize();
    geo::Vector3 b2 = b3.cross(b1);
    tf::Matrix3x3 basis(b1.x(),b2.x(),b3.x(),b1.y(),b2.y(),b3.y(),b1.z(),b2.z(),b3.z());

    // Set the origin and rotation
    pose.setOrigin(v[0]);
    pose.setBasis(basis);

    return true;
}

bool get3DCornerPoints(const cv::Mat& depth_image, const geo::DepthCamera& rasterizer, const std::vector<cv::Point2i>& pts, std::vector<geo::Vector3>& v, unsigned int scale)
{
    v.resize(4);

    for(unsigned int i = 0; i < pts.size(); ++i) {
        float d = depth_image.at<float>(pts[i].y/scale, pts[i].x/scale);
        if (d != d || d == 0) {
            return false;
        }
        v[i] = rasterizer.project2Dto3D(pts[i].x/scale,pts[i].y/scale) * d;
    }

    return true;
}

void getQrCodes(const cv::Mat& rgb_image, std::map<std::string, std::vector<cv::Point2i> >& data)
{
    if (rgb_image.rows == 0 || rgb_image.cols == 0) return;

    // Convert to grayscale
    cv::Mat frame_grayscale;
    cv::cvtColor(rgb_image, frame_grayscale, CV_BGR2GRAY);

    // Obtain image data
    int width = frame_grayscale.cols;
    int height = frame_grayscale.rows;
    uchar *raw = (uchar *)(frame_grayscale.data);

    // Wrap image data
    zbar::Image zbar_image(width, height, "Y800", raw, width * height);

    // Create a zbar reader
    zbar::ImageScanner scanner;

    // Configure the reader
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);

    // Scan the image for barcodes
    scanner.scan(zbar_image);

    // Extract results
    for (zbar::Image::SymbolIterator symbol = zbar_image.symbol_begin(); symbol != zbar_image.symbol_end(); ++symbol) {
        if (symbol->get_data() == "") continue;
        if (symbol->get_location_size() != 4) continue;

        std::vector<cv::Point2i> pts(4);
        for(int i = 0; i < symbol->get_location_size(); ++i) {
            pts[i] = cv::Point2i(symbol->get_location_x(i),symbol->get_location_y(i));
        }

        data[symbol->get_data()] = pts;
    }
}


}
