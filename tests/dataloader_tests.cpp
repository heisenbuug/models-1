/**
 * @file dataloader_tests.cpp
 * @author Kartik Dutt
 *
 * Tests for various functionalities of dataloader.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN // Do not define this anywhere else.
#include <dataloader/dataloader.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(DataLoadersTest);

/**
 * Simple test for Dataloader.
 */
BOOST_AUTO_TEST_CASE(CSVDataLoaderTest)
{
  // Check that dataloader loads only known datasets.
  BOOST_REQUIRE_THROW(DataLoader<>("no-dataset", true), std::runtime_error);

  // Check Load CSV Function for Dataloader.
  Utils::DownloadFile("/datasets/iris.csv", "./../data/iris.csv");
  // Check the file has been downloaded.
  BOOST_REQUIRE(Utils::PathExists("./../data/iris.csv"));

  DataLoader<> irisDataloader;
  irisDataloader.LoadCSV("./../data/iris.csv", true, true, 0.5, false, false,
      0, -1, 1, -1);

  // Check for length and columns of training dataset.
  BOOST_REQUIRE_EQUAL(irisDataloader.TrainLabels().n_cols, 75);
  BOOST_REQUIRE_EQUAL(irisDataloader.TrainLabels().n_rows, 3);

  // Check for validation data as well.
  BOOST_REQUIRE_EQUAL(irisDataloader.ValidFeatures().n_cols, 75);
  BOOST_REQUIRE_EQUAL(irisDataloader.ValidFeatures().n_rows, 4);

  // Check for validation dataset using tuples.
  BOOST_REQUIRE_EQUAL(std::get<1>(irisDataloader.ValidSet()).n_cols, 75);
  BOOST_REQUIRE_EQUAL(std::get<1>(irisDataloader.ValidSet()).n_rows, 3);

  // Check for training dataset using tuples.
  BOOST_REQUIRE_EQUAL(std::get<0>(irisDataloader.TrainSet()).n_cols, 75);
  BOOST_REQUIRE_EQUAL(std::get<0>(irisDataloader.TrainSet()).n_rows, 4);

  Utils::RemoveFile("./../data/iris.csv");
}

/**
 * Simple test for MNIST Dataloader.
 */
BOOST_AUTO_TEST_CASE(MNISTDataLoaderTest)
{
  DataLoader<> dataloader("mnist", true, 0.80);

  // Check for correct dimensions.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_rows, 784);
  BOOST_REQUIRE_EQUAL(dataloader.TestFeatures().n_rows, 784);
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_rows, 784);

  // Check for correct dimensions.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_cols, 8400);
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_cols, 33600);
  BOOST_REQUIRE_EQUAL(dataloader.TestFeatures().n_cols, 28000);

  // Check if we can access both features and labels using
  // TrainSet tuple and ValidSet tuple.
  BOOST_REQUIRE_EQUAL(std::get<0>(dataloader.TrainSet()).n_cols, 8400);
  BOOST_REQUIRE_EQUAL(std::get<1>(dataloader.TrainSet()).n_rows, 1);
  BOOST_REQUIRE_EQUAL(std::get<0>(dataloader.ValidSet()).n_cols, 33600);
  BOOST_REQUIRE_EQUAL(std::get<1>(dataloader.ValidSet()).n_rows, 1);

  // Clean up.
  Utils::RemoveFile("./../data/mnist-dataset/mnist_all.csv");
  Utils::RemoveFile("./../data/mnist-dataset/mnist_all_centroids.csv");
  Utils::RemoveFile("./../data/mnist-dataset/mnist_train.csv");
  Utils::RemoveFile("./../data/mnist-dataset/mnist_test.csv");
  Utils::RemoveFile("./../data/mnist.tar.gz");
}

/**
 * Simple Test for object detection dataloader.
 */
BOOST_AUTO_TEST_CASE(ObjectDetectionDataLoaderFieldTypeTest)
{
  // Download the test dataset.
  Utils::DownloadFile("/datasets/PASCAL-VOC-Test.tar.gz",
    "./../data/PASCAL-VOC-Test.tar.gz", "", false, true,
    "www.mlpack.org", true);

  DataLoader<arma::mat, arma::field<arma::vec>> dataloader;

  // Set paths for dataset.
  std::string basePath = "./../data/PASCAL-VOC-Test/";
  std::string annotaionPath = "Annotations/";
  std::string imagesPath = "Images/";
  double validRatio = 0.2;
  bool shuffle = true;

  // Classes in the dataset.
  std::vector<std::string> classes = {"background", "aeroplane", "bicycle",
      "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow",
      "diningtable", "dog", "horse", "motorbike", "person", "pottedplant",
      "sheep", "sofa", "train", "tvmonitor"};

  // Resize the image to 64 x 64.
  std::vector<std::string> augmentation = {"resize (64, 64)"};
  dataloader.LoadObjectDetectionDataset(basePath + annotaionPath,
      basePath + imagesPath, classes, validRatio, shuffle, augmentation);

  // There are total 136 images in the dataset corresponding to 390 objects.
  // With 20 % data used in validation set, there should be 27 images
  // in training set and 109 image in training dataset.
  BOOST_REQUIRE_EQUAL(dataloader.TrainLabels().n_cols, 109);
  // They correspond to class name, x1, y1, x2, y2.
  BOOST_REQUIRE_EQUAL(dataloader.TrainLabels().n_rows, 1);

  // Check the shape of bounding boxes.
  size_t totalBoundingBoxes = 0;
  for (size_t i = 0; i < dataloader.TrainLabels().n_cols; i++)
  {
    BOOST_REQUIRE_EQUAL(dataloader.TrainLabels()(0, i).n_elem % 5, 0);
    totalBoundingBoxes += dataloader.TrainLabels()(0, i).n_elem / 5;
  }

  // Rows will be equal to shape image depth * image width * image height.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_rows, 64 * 64 * 3);
  // There are 109 images in training set.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_cols, 109);

  // There are 27 images in validation set.
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_cols, 27);
  // They correspond to class name, x1, y1, x2, y2.
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_rows, 1);

  // Each bounding box contains 5 elements corresponding to class label
  // and 4 elements corresponding to bounding box coordinates.
  for (size_t i = 0; i < dataloader.ValidLabels().n_cols; i++)
  {
    BOOST_REQUIRE_EQUAL(dataloader.ValidLabels()(0, i).n_elem % 5, 0);
    totalBoundingBoxes += dataloader.ValidLabels()(0, i).n_elem / 5;
  }

  // There are total 390 objects in the dataset in 136 images.
  BOOST_REQUIRE_EQUAL(totalBoundingBoxes, 390);

  // Rows will be equal to shape image depth * image width * image height.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_rows, 64 * 64 * 3);

  // There are 27 images in the validation set.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_cols, 27);

  // Boundary check.
  validRatio = 1.0;
  dataloader.LoadObjectDetectionDataset(basePath + annotaionPath,
      basePath + imagesPath, classes, validRatio, shuffle, augmentation);

  // There are total 136 images in the set.
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_cols, 136);
  // They correspond to class name, x1, y1, x2, y2.
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_rows, 1);

  // Rows will be equal to shape image depth * image width * image height.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_rows, 64 * 64 * 3);
  // There are total 136 images in the set.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_cols, 136);
}

/**
 * Simple Test for object detection dataloader.
 */
BOOST_AUTO_TEST_CASE(ObjectDetectionDataLoaderMatTypeTest)
{
  // Download the test dataset.
  Utils::DownloadFile("/datasets/PASCAL-VOC-Test.tar.gz",
    "./../data/PASCAL-VOC-Test.tar.gz", "", false, true,
    "www.mlpack.org", true);

  DataLoader<> dataloader;

  // Set paths for dataset.
  std::string basePath = "./../data/PASCAL-VOC-Test/";
  std::string annotaionPath = "Uniform_Annotation/";
  std::string imagesPath = "Images/";
  double validRatio = 0.2;
  bool shuffle = true;

  // Classes in the dataset.
  std::vector<std::string> classes = {"background", "aeroplane", "bicycle",
      "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow",
      "diningtable", "dog", "horse", "motorbike", "person", "pottedplant",
      "sheep", "sofa", "train", "tvmonitor"};

  // Resize the image to 64 x 64.
  std::vector<std::string> augmentation = {"resize (64, 64)"};
  dataloader.LoadObjectDetectionDataset(basePath + annotaionPath,
      basePath + imagesPath, classes, validRatio, shuffle, augmentation);

  // There are total 7 images in the dataset. With 20 % data used in
  // validation set, The training set will have 6 images and validation
  // set will have 1 image.
  BOOST_REQUIRE_EQUAL(dataloader.TrainLabels().n_cols, 6);
  // They correspond to class name, x1, y1, x2, y2.
  BOOST_REQUIRE_EQUAL(dataloader.TrainLabels().n_rows, 5);

  // Rows will be equal to shape image depth * image width * image height.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_rows, 64 * 64 * 3);
  // There are total 6 images in the training set.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_cols, 6);

  // There is 1 image in the validation set.
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_cols, 1);
  // They correspond to class name, x1, y1, x2, y2.
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_rows, 5);

  // Rows will be equal to shape image depth * image width * image height.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_rows, 64 * 64 * 3);
  // There is 1 image in the validation set.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_cols, 1);
}

BOOST_AUTO_TEST_CASE(LoadImageDatasetFromDirectoryTest)
{
  // Download the test dataset.
  Utils::DownloadFile("/datasets/cifar-test.tar.gz",
    "./../data/cifar-test.tar.gz", "", false, true,
    "www.mlpack.org", true);

  DataLoader<> dataloader;
  Utils::ExtractFiles("./../data/cifar-test.tar.gz", "./../data/");
  dataloader.LoadImageDatasetFromDirectory("./../data/cifar-test/",
      32, 32, 3, true);

  // Check correctness of Training data.
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_cols, 800);
  BOOST_REQUIRE_EQUAL(dataloader.TrainFeatures().n_rows, 32 * 32 * 3);

  BOOST_REQUIRE_EQUAL(dataloader.TrainLabels().n_cols, 800);
  BOOST_REQUIRE_EQUAL(dataloader.TrainLabels().n_rows, 1);

  // Check correctness of Validation data.
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_cols, 200);
  BOOST_REQUIRE_EQUAL(dataloader.ValidFeatures().n_rows, 32 * 32 * 3);

  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_cols, 200);
  BOOST_REQUIRE_EQUAL(dataloader.ValidLabels().n_rows, 1);
}

BOOST_AUTO_TEST_SUITE_END();
