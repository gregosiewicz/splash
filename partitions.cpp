#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/math/distributions/normal.hpp>

typedef boost::multiprecision::uint512_t bigInt;
typedef boost::multiprecision::cpp_dec_float_100 bigFloat;

template <typename T>
class vector3d {
    public:
        vector3d(size_t d1 = 0, size_t d2 = 0, size_t d3 = 0, T const & t = T()) :
            d1(d1), d2(d2), d3(d3), data(d1 * d2 * d3, t)
        {}

        T & operator()(size_t i, size_t j, size_t k) {
            return data[i * d2 * d3 + j * d3 + k];
        }

        T const & operator()(size_t i, size_t j, size_t k) const {
            return data[i* d2 * d3 + j * d3 + k];
        }

    private:
        size_t d1, d2, d3;
        std::vector<T> data;
};

class Partitions {
    private:
        vector3d<bigInt> partitionsMatrix;

    public:
        size_t minNumber;
        size_t maxNumber;
        size_t numPartsMin;
        size_t numPartsMax;
        size_t partSizeMin;
        size_t partSizeMax;
        std::vector<bigInt> cumulativePartitions;

        Partitions(size_t minNumber, size_t maxNumber, size_t numPartsMin, size_t numPartsMax, size_t partSizeMin, size_t partSizeMax):
            minNumber(minNumber),
            maxNumber(maxNumber),
            numPartsMin(numPartsMin),
            numPartsMax(numPartsMax),
            partSizeMin(partSizeMin),
            partSizeMax(partSizeMax)
        {
            partitionsMatrix = vector3d<bigInt>(maxNumber + 1, numPartsMax + 1, partSizeMax + 1, 0);
            cumulativePartitions = std::vector<bigInt>(maxNumber + 1, 0);
        }

        bigInt numberOfPartitions(size_t number, size_t parts, size_t partSizeMax) {
        // Count the number of partitions of the number into parts parts with restrictions
        // on each part to be at least 1 and at most partSizeMax

            // already counted
            if (partitionsMatrix(number, parts, partSizeMax) != 0)
                return partitionsMatrix(number, parts, partSizeMax);
            
            // partition is impossible, the number is too large or too small
            if (partSizeMax * parts < number || number < parts) 
                return 0;

            // only one possibility: all ones or all partSizeMax
            if (partSizeMax * parts == number || number <= parts + 1)
                return (partitionsMatrix(number, parts, partSizeMax) = 1);
            
            if (parts == 1)
                return (partitionsMatrix(number, parts, partSizeMax) = 1);
            
            if (parts == 2) {
                if (partSizeMax * 2 >= number) {
                    // partition is possible
                    partSizeMax = std::min(partSizeMax, number - 1);
                    return (partitionsMatrix(number, parts, partSizeMax) = number / parts - (number - 1 - partSizeMax));
                } else {
                    return 0;
                }
            }

            // real counting, use a formula from the paper
            bigInt count = 0;
            size_t iterNum = number / parts;
            for (size_t i = 0; i < iterNum; ++i) {
                count += (partitionsMatrix(number - 1, parts - 1, partSizeMax) = numberOfPartitions(number - 1, parts - 1, partSizeMax));
                number -= parts;
                --partSizeMax;
            }
            
            return (partitionsMatrix(number, parts, partSizeMax) = count);
        }

        // Return number of partitions with parts bounded by partSizeMin and partSizeMax
        bigInt numberOfPartitions(size_t number, size_t parts, size_t partSizeMin, size_t partSizeMax) {
            return numberOfPartitions(number - parts * (partSizeMin - 1), parts, partSizeMax - partSizeMin + 1);
        }

        // calculate partitions and cumulative partitions
        void calculateCumulativePartitions() {
            for(size_t number = minNumber; number <= maxNumber; ++number) {
                // if(number % 100 == 0) std::cout << number << std::endl;
                for(size_t parts = (size_t)ceil(number / (double)partSizeMax); parts <= number / partSizeMin; ++parts) {
                    cumulativePartitions[number] += numberOfPartitions(number, parts, partSizeMin, partSizeMax);
		        }
	        }
        }
};


struct Parameters {
    size_t eMean;
    double eStd;
    double eMin;
    double eMax;
    size_t eMinDiscrete;
    size_t eMaxDiscrete;
    size_t numPartsMin;
    size_t numPartsMax;
    size_t partSizeMin;
    size_t partSizeMax;

    Parameters(char* argv[]) {
        eMean = atoi(argv[1]);
        eStd = atof(argv[2]);
        eMin = atof(argv[3]);
        eMax = atof(argv[4]);
        partSizeMin = atoi(argv[5]);
        partSizeMax = atoi(argv[6]);
        eMinDiscrete = (size_t)ceil(eMin);
        eMaxDiscrete = (size_t)floor(eMax);
        numPartsMin = (size_t)ceil(eMin / (double)partSizeMax);
        numPartsMax = (size_t)floor(eMax / (double)partSizeMin);
    };
};

class DiscreteDistribution {
    public:
        std::vector<bigFloat> distribution;
        size_t min;
        size_t max;

        void printDistribution() {
                std::cout << "no,prob" << std::endl;
                for(size_t i = min; i <= max; ++i)
                    std::cout << i << "," << std::setprecision(10) << distribution[i] << std::endl;
        }
};

class DiscreteNormalDistribution: public DiscreteDistribution {
    public:
        DiscreteNormalDistribution(size_t mean, double std, double minValue, double maxValue) {
            // discrete normal distribution restricted to [ceil(minValue),floor(maxValue)]
            // with mean and std equals to mean and std
            min = ceil(minValue);
            max = floor(maxValue);

            distribution = std::vector<bigFloat>(max + 1, 0);
            
            boost::math::normal normal(mean, std);
            double normalization = cdf(normal, maxValue) - cdf(normal, minValue);

            distribution[min] = cdf(normal, min + 0.5) - cdf(normal, minValue);
            distribution[max] = cdf(normal, maxValue) - cdf(normal, max - 0.5);

            for(size_t i = min + 1; i < max; ++i) {
                distribution[i] = (cdf(normal, i + 0.5) - cdf(normal, i - 0.5)) / normalization;
            }
        }
};

class IntegerPartitionDistribution: public DiscreteDistribution {
    public:
        // calculate integer partition distribution conditional on a given discrete distribution
        IntegerPartitionDistribution(Partitions partitions, DiscreteDistribution discreteDistribution) {
            min = partitions.numPartsMin;
            max = partitions.numPartsMax;
            distribution = std::vector<bigFloat>(partitions.maxNumber + 1, 0);

            partitions.calculateCumulativePartitions();

            for(size_t number = partitions.minNumber; number <= partitions.maxNumber; ++number) {
                for(size_t parts = (size_t)ceil(number / (double)partitions.partSizeMax); parts <= number / partitions.partSizeMin; ++parts) {
                    distribution[parts] += (bigFloat)partitions.numberOfPartitions(number, parts, partitions.partSizeMin, partitions.partSizeMax)
                                            / (bigFloat)partitions.cumulativePartitions[number]
                                            * discreteDistribution.distribution[number];
		        }
	        }
        }
};


int main(int argc, char* argv[]) {

    if(argc != 7) {
        std::cout << "Execution: ./partitions energyMean energyStd energyMin energyMax partSizeMin partSizeMax" << std::endl;
        return 0;
    }

    Parameters parameters(argv);
    DiscreteNormalDistribution discreteNormalDistribution(parameters.eMean, parameters.eStd, parameters.eMin, parameters.eMax);
    Partitions partitions(parameters.eMinDiscrete, parameters.eMaxDiscrete,
                          parameters.numPartsMin, parameters.numPartsMax,
                          parameters.partSizeMin, parameters.partSizeMax);
    IntegerPartitionDistribution integerPartitionDistribution(partitions, discreteNormalDistribution);
    integerPartitionDistribution.printDistribution();

	return 0;
}
