#include "aruco.h"


ArucoDetector::ArucoDetector(std::shared_ptr<RealSense> camera, cv::aruco::PREDEFINED_DICTIONARY_NAME dictionary_name, float marker_size){
    this->camera = camera;
    this->marker_size = marker_size;
    dictionary = cv::aruco::getPredefinedDictionary(dictionary_name);
    parameters = cv::aruco::DetectorParameters::create();
}

void ArucoDetector::Detect(cv::Mat &image, std::vector<int> &ids, std::vector<Vector3f> &rvecs, std::vector<Vector3f> &tvecs, bool draw) {
    std::vector<int> ids_cv;
    std::vector<std::vector<cv::Point2f>> marker_corners, rejected_candidates;
    cv::aruco::detectMarkers(image, dictionary, marker_corners, ids_cv, parameters, rejected_candidates, camera->GetKForOpenCV(), camera->GetDForOpenCV());
    
    ids.clear();
    for (size_t i = 0; i < ids_cv.size(); i ++) {
        ids.push_back(ids_cv[i]);
    }
    if (ids.size() > 0) {
        std::vector<cv::Vec3d> rvecs_cv, tvecs_cv;
        cv::aruco::estimatePoseSingleMarkers(marker_corners, marker_size, camera->GetKForOpenCV(), camera->GetDForOpenCV(), rvecs_cv, tvecs_cv);

        rvecs.clear();
        tvecs.clear();
        for (size_t i = 0; i < ids.size(); i ++) {
            Vector3f rvec_eigen, tvec_eigen;
            rvec_eigen(0) = rvecs_cv[i][0];
            rvec_eigen(1) = rvecs_cv[i][1];
            rvec_eigen(2) = rvecs_cv[i][2];
            tvec_eigen(0) = tvecs_cv[i][0];
            tvec_eigen(1) = tvecs_cv[i][1];
            tvec_eigen(2) = tvecs_cv[i][2];
            rvecs.push_back(rvec_eigen);
            tvecs.push_back(tvec_eigen);
        }
        
        if (draw) {
            cv::aruco::drawDetectedMarkers(image, marker_corners, ids);
            for (size_t i = 0; i < ids.size(); i ++) {
                cv::aruco::drawAxis(image, camera->GetKForOpenCV(), camera->GetDForOpenCV(), rvecs_cv[i], tvecs_cv[i], 0.1);
            }
        }
    }
}

ArucoMarker::ArucoMarker(int id, int num, Matrix4f T_M_P) {
    this->id = id;
    this->num = num;
    this->T_M_P = T_M_P;
    Vector6f zero_vec;
    zero_vec << 0, 0, 0, 0, 0, 0;
    float fac = 1.0 / num;
    for (int i = 0; i < num; i ++) {
        rt_vecs.push_back(zero_vec);
        weights.push_back(fac);
    }
}

void ArucoMarker::Update(Vector3f rvec, Vector3f tvec) {
    Vector6f rt_vec;
    rt_vec << rvec(0), rvec(1), rvec(2), tvec(0), tvec(1), tvec(2);
    
    // Add
    rt_vecs.erase(rt_vecs.begin());
    rt_vecs.push_back(rt_vec);

    // Update Weights
    float sum = 0.0;
    for (int i = 0; i < num; i ++) {
        float norm = (rt_vec - rt_vecs[i]).norm();
        if (norm < 1e-6) {
            weights[i] = 1e6;
        } else {
            weights[i] = 1.0 / norm;
        }
        sum += weights[i];
    }
    for (int i = 0; i < num; i ++) {
        weights[i] /= sum;
    }
}

Vector6f ArucoMarker::GetRtVec() {
    Vector6f average = Vector6f::Zero();
    for (auto i = 0; i < num; i ++) {
        average += rt_vecs[i] * weights[i];
    }
    return average;
}

void ArucoMarker::SetT_M_P(Vector3f rvec, Vector3f tvec) {
    Vector6f rt_vec;
    rt_vec << rvec(0), rvec(1), rvec(2), tvec(0), tvec(1), tvec(2);
    T_M_P = RtVec2T(rt_vec);
}
