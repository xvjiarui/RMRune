#include <opencv2/opencv.hpp>
using namespace cv;
Scalar colorTab[] =     //10个颜色
{
	Scalar(0, 0, 255),
	Scalar(0, 255, 0),
	Scalar(255, 100, 100),
	Scalar(255, 0, 255),
	Scalar(0, 255, 255),
	Scalar(255, 0, 0),
	Scalar(255, 255, 0),
	Scalar(255, 0, 100),
	Scalar(100, 100, 100),
	Scalar(50, 125, 125)
};

class ClusterPixels
{
private:
	Mat image;			//待聚类图像
	Mat labels;			//聚类后的标签
	int clusterCounts;	//分类数,不得大于10，只是颜色定义只有10类，并不是算法限制

public:
	ClusterPixels() :clusterCounts(0){}
	ClusterPixels(const Mat& src, int clusters = 5) :clusterCounts(clusters){ image = src.clone(); }

	void setImage(const Mat& src){ image = src.clone(); };
	void setClusters(int clusters){ clusterCounts = clusters; }

	Mat getLabels()	{return labels;	};		//返回聚类后的标签

	Mat clusterGrayImageByKmeans()
	{
		//转换成灰度图
		if (image.channels() != 1)
			cvtColor(image, image, COLOR_BGR2GRAY);

		int rows = image.rows;
		int cols = image.cols;
		
		//保存聚类后的图片
		Mat clusteredMat(rows, cols, CV_8UC3);
		clusteredMat.setTo(Scalar::all(0));

		Mat pixels(rows*cols, 1, CV_32FC1);	//pixels用于保存所有的灰度像素

		for (int i = 0; i < rows;++i)
		{
			const uchar *idata = image.ptr<uchar>(i);
			float *pdata = pixels.ptr<float>(0);
			for (int j = 0; j < cols;++j)
			{
				pdata[i*cols + j] = idata[j];
			}
		}

		kmeans(pixels, clusterCounts, labels, TermCriteria(TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 0), 5, KMEANS_PP_CENTERS);

		for (int i = 0; i < rows;++i)
		{
			for (int j = 0; j < cols;++j)
			{
				circle(clusteredMat, Point(j,i), 1, colorTab[labels.at<int>(i*cols + j)]);		//标记像素点的类别，颜色区分
			}
		}

		return clusteredMat;
	}

	Mat clusterColorImageByKmeans()
	{
		assert(image.channels() != 1);

		int rows = image.rows;
		int cols = image.cols;
		int channels = image.channels();

		//保存聚类后的图片
		Mat clusteredMat(rows, cols, CV_8UC3);
		clusteredMat.setTo(Scalar::all(0));

		Mat pixels(rows*cols + 1, 1, CV_32FC3);	//pixels用于保存所有的灰度像素
		pixels.setTo(Scalar::all(0));

		for (int i = 0; i < rows; ++i)
		{
			const uchar *idata = image.ptr<uchar>(i);
			float *pdata = pixels.ptr<float>(0);

			for (int j = 0; j < cols*channels; ++j)
			{
					pdata[i*cols*channels + j] = saturate_cast<float>(idata[j]);			
			}
		}

		kmeans(pixels, clusterCounts, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 0), 5, KMEANS_PP_CENTERS);
        return labels;
        /*
        cout<<labels<<endl;

		for (int i = 0; i < rows; ++i)
		{
			for (int j = 0; j < cols*channels; j += channels)
			{
				circle(clusteredMat, Point(j/channels,i), 1, colorTab[labels.at<int>(i*cols + (j/channels))]);		//标记像素点的类别，颜色区分
			}
		}

		return clusteredMat;
        */
	}
};
