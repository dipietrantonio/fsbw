#include <math.h>
#include "stats.h"


float mean(float *values, unsigned int n){
    if(n == 0) return 0;
    float mean = 0;
    for(unsigned int i = 0; i < n; i++){
        mean += values[i];
    }
    return mean / n;
}


float variance(float *values, unsigned int n){
    if(n == 0) return 0;
    float var = 0;
    float mean_val = mean(values, n);
    for(unsigned int i = 0; i < n; i++){
        var += powf(mean_val - values[i], 2);
    }
    return var / n;
}

float stddev(float *values, unsigned int n){
    if(n == 0) return 0;
    return sqrt(variance(values, n));
}