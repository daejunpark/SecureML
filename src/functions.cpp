#include "functions.h"

namespace SecureML
{
	/****************************************************************************/
	double innerproduct(double* vec1, double* vec2, long size) {
		double ip = 0.;
		for(long i = 0; i < size; i++) {
			ip += vec1[i] * vec2[i];
		}
		return ip;
	}
	/****************************************************************************/
	double** zDataFromFile(string path, long& factorDim, long& sampleDim, bool isfirst) {
		vector< vector<double> > zline;
		factorDim = 1; 	// dimension of x
		sampleDim = 0;	// number of samples
		ifstream openFile(path.data());
		if(openFile.is_open()) {
			string line, temp;
			getline(openFile, line);
			long i;
			size_t start = 0;
			size_t end = 0;
			for(i = 0; i < line.length(); ++i) {
				if(line[i] == ',' ) factorDim++;
			}
			while(getline(openFile, line)) {
				vector<double> vecline;
				do {
					end = line.find_first_of (',', start);
					temp = line.substr(start,end);
					vecline.push_back(atof(temp.c_str()));
					start = end + 1;
				} while(start);
				zline.push_back(vecline);
				sampleDim++;
			}
		} else {
			cout << "Error: cannot read file" << endl;
		}
		double** zData = new double*[sampleDim];
		if(isfirst) {
			for(long j = 0; j < sampleDim; ++j) {
				double* zj = new double[factorDim];
				zj[0] = 2 * zline[j][0] - 1;
				for(long i = 1; i < factorDim; ++i){
					zj[i] = zj[0] * zline[j][i];
				}
				zData[j] = zj;
			}
		} else {
			for(long j = 0; j < sampleDim; ++j) {
				double* zj = new double[factorDim];
				zj[0] = 2 * zline[j][factorDim - 1] - 1;
				for(long i = 1; i < factorDim; ++i) {
					zj[i] = zj[0] * zline[j][i-1];
				}
				zData[j] = zj;
			}
		}
		return zData;
	}
	/****************************************************************************/
	void normalizeZData(double** zData, long factorDim, long sampleDim) {
		for (long i = 0; i < factorDim; ++i) {
			double m = 0.0;
			for (long j = 0; j < sampleDim; ++j) {
				if(m < abs(zData[j][i])) m = abs(zData[j][i]);
			}
			if(m > 1e-10) {
				for (long j = 0; j < sampleDim; ++j) {
					zData[j][i] = zData[j][i] / m;
				}
			}
		}
	}
	/****************************************************************************/
	void shuffleZData(double** zData, long factorDim, long sampleDim) {
		//srand(time(NULL));
		srand(1111); //> fix seed for fair comparison
		double* tmp = new double[factorDim];
		for (long i = 0; i < sampleDim; ++i) {
			long idx = i + rand() / (RAND_MAX / (sampleDim - i) + 1);
			copy(zData[i], zData[i] + factorDim, tmp);
			copy(zData[idx], zData[idx] + factorDim, zData[i]);
			copy(tmp, tmp + factorDim, zData[idx]);
		}
		delete[] tmp;
	}
	/****************************************************************************/
	void testProbAndYval(string path, double* wData, bool isfirst) {
		long sampleNum;
		long factorNum;
		double** zData = zDataFromFile(path, factorNum, sampleNum, isfirst);
		double* yval = new double[sampleNum]();
		double* prob = new double[sampleNum]();
		for(long i = 0; i < sampleNum; i++) {
			if(zData[i][0] > 0) yval[i] = 1;
			else yval[i] = 0;
			double sum = zData[i][0] * innerproduct(wData, zData[i], factorNum);
			prob[i] = 1. / (1. + exp(-sum));
		}
		ofstream openFile("testResult.csv");
		openFile << "Prob, Y" << endl;
		for(long i = 0; i < sampleNum; i++) {
			openFile << prob[i] << ", " << yval[i] << endl;
		}
		openFile.close();
		delete[] yval;
		delete[] prob;
	}
	/****************************************************************************/
	void testAUROC(double& auc, double& accuracy, string path, double* wData, bool isfirst) {

		long sampleNum;
		long factorNum;
		double** zData = zDataFromFile(path, factorNum, sampleNum, isfirst);
		normalizeZData(zData, factorNum, sampleNum);
		
		cout << "wData = [";
		for(long i = 0; i < 10; i++) {
			cout << wData[i] << ',';
			if(i == 9) cout << wData[i] << "]" << endl;
		}

		long TN = 0, FP = 0;
		vector<double> thetaTN(0);
		vector<double> thetaFP(0);

	   	for(long i = 0; i < sampleNum; ++i){
	    	if(zData[i][0] > 0)
			{
	            if(innerproduct(zData[i], wData, factorNum) < 0){
					TN++;
				}
	            thetaTN.push_back(zData[i][0] * innerproduct(zData[i] + 1, wData + 1, factorNum - 1));
	        } else {
		        if(innerproduct(zData[i], wData, factorNum) < 0){
					FP++;
				}
	        	thetaFP.push_back(zData[i][0] * innerproduct(zData[i] + 1, wData + 1, factorNum - 1));
	        }
	    }
		accuracy = (double)(sampleNum - TN - FP) / sampleNum;
		cout << "Accuracy: " << accuracy << endl;
		auc = 0.0;
	    if(thetaFP.size() == 0 || thetaTN.size() == 0) {
	        cout << "n_test_yi = 0 : cannot compute AUC" << endl;
	    } else {
	    	for(long i = 0; i < thetaTN.size(); ++i){
	        	for(long j = 0; j < thetaFP.size(); ++j){
	            	if(thetaFP[j] <= thetaTN[i]) auc++;
	        	}
	    	}
	    	auc /= thetaTN.size() * thetaFP.size();
	    	cout << "AUC: " << auc << endl;
	    }
		vector<double>().swap(thetaTN);
		vector<double>().swap(thetaFP);
	}
	/****************************************************************************/
}


