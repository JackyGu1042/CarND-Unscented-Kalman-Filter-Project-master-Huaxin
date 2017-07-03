#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>
#include <fstream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 30;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 30;

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
    // State dimension
    n_x_ = 5;
    
    // Augmented state dimension
    n_aug_ = n_x_ + 2;
    
    // Sigma point spreading parameter
    lambda_ = 3 - n_aug_;
    
    // predicted sigma points matrix
    Xsig_pred_ = MatrixXd(n_x_, 2*n_aug_+1);
    
    // Weights of sigma points
    weights_ = VectorXd(2*n_aug_+1);
    
    P_<<1,0,0,0,0,
		0,1,0,0,0,
		0,0,1,0,0,
		0,0,0,1,0,
		0,0,0,0,1;
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
     cout << "ProcessMeasurement()->Begin" << endl;
        
    /*****************************************************************************
    *  Initialization
    ****************************************************************************/
    
	if (!is_initialized_) {

        cout << "ProcessMeasurement()->Initialization" << endl;
        
        if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
            /**
             Convert radar from polar to cartesian coordinates and initialize state.
             */
            cout << "ProcessMeasurement()->Initialization->Radar" << endl;
            
            float rho = meas_package.raw_measurements_[0];
            float theta = meas_package.raw_measurements_[1];
            float rhodot = meas_package.raw_measurements_[2];
            
            while (theta> M_PI) theta -=2.*M_PI;
            while (theta<-M_PI) theta +=2.*M_PI;
            
            x_ << rho*cos(theta), rho*sin(theta), rhodot, theta, 0;
            
            time_us_ = meas_package.timestamp_;
            is_initialized_ = true;
            return;
        }
        else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {
            /**
             Initialize state.
             */
            cout << "ProcessMeasurement()->Initialization->Laser" << endl;
            
            float p_x = meas_package.raw_measurements_[0];
            float p_y = meas_package.raw_measurements_[1];
            float theta = atan2(p_y,p_x);
            
            while (theta> M_PI) theta -=2.*M_PI;
            while (theta<-M_PI) theta +=2.*M_PI;
            
            x_ << p_x, p_y, 0, 0, 0;
            
            time_us_ = meas_package.timestamp_;
            is_initialized_ = true;
            return;
        }
            
        // done initializing, no need to predict or update
        is_initialized_ = true;
        return;
    }
        
    /*****************************************************************************
     *  Prediction
     ****************************************************************************/
    
    cout << "ProcessMeasurement()->Time stamp: " << meas_package.timestamp_ << endl;
    cout << "ProcessMeasurement()->Prediction" << endl;
    
    float dt = (meas_package.timestamp_ - time_us_) / 1000000.0;	//dt - expressed in seconds
    time_us_ = meas_package.timestamp_;

    Prediction(dt);
    
    /*****************************************************************************
     *  Update
     ****************************************************************************/
    
    cout << "ProcessMeasurement()->Update" << endl;
    
    if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
        cout << "ProcessMeasurement()->Update->Radar" << endl;
        
        UpdateRadar(meas_package);
    }
    else if (meas_package.sensor_type_ == MeasurementPackage::LASER){
        cout << "ProcessMeasurement()->Update->Laser" << endl;

        UpdateLidar(meas_package);
    }
    
    // print the output
    cout << "x_ = " << x_ << endl;
    cout << "P_ = " << P_ << endl;
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
    std::cout << "Prediction()->Begin" << std::endl;
    
    //create augmented mean vector
    VectorXd x_aug_ = VectorXd(n_aug_);
    
    //create augmented state covariance
    MatrixXd P_aug_ = MatrixXd(n_aug_, n_aug_);
    
    //create sigma point matrix
    MatrixXd Xsig_aug_ = MatrixXd(n_aug_, 2 * n_aug_ + 1);
    
    //create augmented mean state
    x_aug_.head(5) = x_;
    x_aug_(5) = 0;
    x_aug_(6) = 0;
    
    std::cout << "Prediction()->Create x_aug_:" << x_aug_ << std::endl;
    
    //create augmented covariance matrix
    P_aug_.fill(0.0);
    P_aug_.topLeftCorner(5,5) = P_;
    P_aug_(5,5) = std_a_*std_a_;
    P_aug_(6,6) = std_yawdd_*std_yawdd_;
    
    //create square root matrix
    MatrixXd L = P_aug_.llt().matrixL();
    
    //create augmented sigma points
    Xsig_aug_.col(0)  = x_aug_;
    for (int i = 0; i< n_aug_; i++)
    {
        Xsig_aug_.col(i+1)       = x_aug_ + sqrt(lambda_+n_aug_) * L.col(i);
        Xsig_aug_.col(i+1+n_aug_) = x_aug_ - sqrt(lambda_+n_aug_) * L.col(i);
    }
    
    //print result
    //std::cout << "UKF::Prediction()->Xsig_aug_ = " << Xsig_aug_ << std::endl;
    
    //predict sigma points
    for (int i = 0; i< 2*n_aug_+1; i++)
    {
        std::cout << "Prediction()->Estimate i = " << i << std::endl;
		
		//extract values for better readability
        double p_x = Xsig_aug_(0,i);
        double p_y = Xsig_aug_(1,i);
        double v = Xsig_aug_(2,i);
        double yaw = Xsig_aug_(3,i);
        double yawd = Xsig_aug_(4,i);
        double nu_a = Xsig_aug_(5,i);
        double nu_yawdd = Xsig_aug_(6,i);
        
        //predicted state values
        double px_p, py_p;
        
        //avoid division by zero
        if (fabs(yawd) > 0.001) {
            px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
            py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
        }
        else {
            px_p = p_x + v*delta_t*cos(yaw);
            py_p = p_y + v*delta_t*sin(yaw);
        }
        
		std::cout << "Prediction()->Estimate px_p = " << px_p << std::endl;
		std::cout << "Prediction()->Estimate py_p = " << py_p << std::endl;
		
        double v_p = v;
        double yaw_p = yaw + yawd*delta_t;
        double yawd_p = yawd;
        
        //add noise
        px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
        py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
        v_p = v_p + nu_a*delta_t;
        
        yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
        yawd_p = yawd_p + nu_yawdd*delta_t;
        
		std::cout << "Prediction()->Estimate yaw_p = " << yaw_p << std::endl;
		std::cout << "Prediction()->Estimate yawd_p = " << yawd_p << std::endl;
		
        //write predicted sigma point into right column
        Xsig_pred_(0,i) = px_p;
        Xsig_pred_(1,i) = py_p;
        Xsig_pred_(2,i) = v_p;
        Xsig_pred_(3,i) = yaw_p;
        Xsig_pred_(4,i) = yawd_p;
    }
    
    //print result
    std::cout << "Prediction()->Xsig_pred_ = " << Xsig_pred_ << std::endl;
    
    // set weights
    double weight_0 = lambda_/(lambda_+n_aug_);
    weights_(0) = weight_0;
    for (int i=1; i<2*n_aug_+1; i++) {  //2n+1 weights
        double weight = 0.5/(n_aug_+lambda_);
        weights_(i) = weight;
    }
    
	std::cout << "Prediction()->weights_ = " << weights_ << std::endl;
	
    //predicted state mean
    x_.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points
        x_ = x_+ weights_(i) * Xsig_pred_.col(i);
    }
    
    //predicted state covariance matrix
    P_.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points
        
        // state difference
        VectorXd x_diff = Xsig_pred_.col(i) - x_;
        //angle normalization
        while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
        while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
        
        P_ = P_ + weights_(i) * x_diff * x_diff.transpose() ;
    }

    //print result
    //std::cout << "Predicted state" << std::endl;
    //std::cout << x_ << std::endl;
    //std::cout << "Predicted covariance matrix" << std::endl;
    //std::cout << P_ << std::endl;
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
    //set measurement dimension, radar can measure position_x and position_y
    int n_z_ = 2;
    
    MatrixXd Zsig_ = MatrixXd(n_z_, 2 * n_aug_ + 1);
    
    //transform sigma points into measurement space
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
        
        // extract values for better readibility
        double p_x = Xsig_pred_(0,i);
        double p_y = Xsig_pred_(1,i);
        double v  = Xsig_pred_(2,i);
        double yaw = Xsig_pred_(3,i);
        
        double v1 = cos(yaw)*v;
        double v2 = sin(yaw)*v;
        
        // measurement model
        Zsig_(0,i) = p_x;  //p_x
        Zsig_(1,i) = p_y;  //p_y
    }
    
    //mean predicted measurement
    VectorXd z_pred_ = VectorXd(n_z_);
    z_pred_.fill(0.0);
    for (int i=0; i < 2*n_aug_+1; i++) {
        z_pred_ = z_pred_ + weights_(i) * Zsig_.col(i);
    }
	
    std::cout << "UpdateLidar()->z_pred: " << z_pred_ << std::endl;
	
    //measurement covariance matrix S
    MatrixXd S_ = MatrixXd(n_z_,n_z_);
    S_.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
        //residual
        VectorXd z_diff = Zsig_.col(i) - z_pred_;
        
        S_ = S_ + weights_(i) * z_diff * z_diff.transpose();
    }
    
    //add measurement noise covariance matrix
    MatrixXd R_ = MatrixXd(n_z_,n_z_);
    R_ <<    std_laspx_*std_laspx_, 0,
    0, std_laspy_*std_laspy_;
    S_ = S_ + R_;
    
    //print result
    std::cout << "UpdateLidar()->S: " << S_ << std::endl;
  
    //create matrix for cross correlation Tc
    MatrixXd Tc_ = MatrixXd(n_x_, n_z_);
    
    //calculate cross correlation matrix
    Tc_.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
        
        //residual
        VectorXd z_diff = Zsig_.col(i) - z_pred_;
        //angle normalization
        while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
        while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
        
        // state difference
        VectorXd x_diff = Xsig_pred_.col(i) - x_;
        //angle normalization
        while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
        while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
        
        Tc_ = Tc_ + weights_(i) * x_diff * z_diff.transpose();
    }
    
    //Kalman gain K;
    MatrixXd K_ = Tc_ * S_.inverse();
    
    //create example vector for incoming radar measurement
    VectorXd z_ = VectorXd(n_z_);
    
    z_ = meas_package.raw_measurements_;
    //residual
    VectorXd z_diff = z_ - z_pred_;
    
    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
    
    //update state mean and covariance matrix
    x_ = x_ + K_ * z_diff;
    P_ = P_ - K_*S_*K_.transpose();
	
	//Normalized Innovation Squared(NIS) calculation
	double diff_NIS = (z_ - z_pred_);
	double NIS = diff_NIS.transpose()*S_*diff_NIS;
	
	std::cout << "UpdateLidar()->NIS: " << NIS << std::endl;
	ofstream NIS_file;
	NIS_file.open ("NIS_file.txt");
	NIS_file << NIS;
	NIS_file.close();
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
    //set measurement dimension, radar can measure r, phi, and r_dot
    int n_z_ = 3;
    
    MatrixXd Zsig_ = MatrixXd(n_z_, 2 * n_aug_ + 1);
    
    //transform sigma points into measurement space
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
        
        // extract values for better readibility
        double p_x = Xsig_pred_(0,i);
        double p_y = Xsig_pred_(1,i);
        double v  = Xsig_pred_(2,i);
        double yaw = Xsig_pred_(3,i);
        
        double v1 = cos(yaw)*v;
        double v2 = sin(yaw)*v;
        
        // measurement model
        Zsig_(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //r
        Zsig_(1,i) = atan2(p_y,p_x);                                 //phi
        Zsig_(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //r_dot
    }
    
    //mean predicted measurement
    VectorXd z_pred_ = VectorXd(n_z_);
    z_pred_.fill(0.0);
    for (int i=0; i < 2*n_aug_+1; i++) {
        z_pred_ = z_pred_ + weights_(i) * Zsig_.col(i);
    }
	
	std::cout << "UpdateLidar()->z_pred: " << z_pred_ << std::endl;
    
    //measurement covariance matrix S
    MatrixXd S_ = MatrixXd(n_z_,n_z_);
    S_.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
        //residual
        VectorXd z_diff = Zsig_.col(i) - z_pred_;
        
        //angle normalization
        while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
        while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
        
        S_ = S_ + weights_(i) * z_diff * z_diff.transpose();
    }
    
    //add measurement noise covariance matrix
    MatrixXd R_ = MatrixXd(n_z_,n_z_);
    R_ <<    std_radr_*std_radr_, 0, 0,
    0, std_radphi_*std_radphi_, 0,
    0, 0,std_radrd_*std_radrd_;
    S_ = S_ + R_;
    
    //print result
    std::cout << "UpdateLidar()->S: " << S_ << std::endl;

    //create matrix for cross correlation Tc
    MatrixXd Tc_ = MatrixXd(n_x_, n_z_);
    
    //calculate cross correlation matrix
    Tc_.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
        
        //residual
        VectorXd z_diff = Zsig_.col(i) - z_pred_;
        //angle normalization
        while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
        while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
        
        // state difference
        VectorXd x_diff = Xsig_pred_.col(i) - x_;
        //angle normalization
        while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
        while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
        
        Tc_ = Tc_ + weights_(i) * x_diff * z_diff.transpose();
    }
    
    //Kalman gain K;
    MatrixXd K_ = Tc_ * S_.inverse();
    
    //create example vector for incoming radar measurement
    VectorXd z_ = VectorXd(n_z_);
    
    z_ = meas_package.raw_measurements_;
    //residual
    VectorXd z_diff = z_ - z_pred_;
    
    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
    
    //update state mean and covariance matrix
    x_ = x_ + K_ * z_diff;
    P_ = P_ - K_*S_*K_.transpose();
	
	//Normalized Innovation Squared(NIS) calculation
	double diff_NIS = (z_ - z_pred_);
	double NIS = diff_NIS.transpose()*S_*diff_NIS;
	
	std::cout << "UpdateLidar()->NIS: " << NIS << std::endl;
	ofstream NIS_file;
	NIS_file.open ("NIS_file.txt");
	NIS_file << NIS;
	NIS_file.close();
}
