#include <stdio.h>
#include <stdint.h>
#include <string.h>

uint16_t greatestCommonDivisor(uint16_t a, uint16_t b){
    while(b != 0){
        uint16_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

uint32_t lowestCommonDenominator(uint32_t a, uint32_t b){
    return (a * b) / greatestCommonDivisor(a, b);
}

uint32_t downsampleDiscreteArray(uint16_t *array, uint16_t arrLen, uint16_t prevSamplePeriod, uint16_t newSamplePeriod){
    uint32_t samplesWritten = 0;
    int32_t phase = 0;
    uint32_t carry = 0;

    // Downsampling starts from the most recent sample
    for(uint16_t i = 0; i < arrLen; i++){
        uint32_t prevValue = array[i];

        phase += prevSamplePeriod;
        carry += prevValue;
        // printf("in %2d - Phase: %d, Value: %d, Carry: %d\n", i, phase, prevValue, carry);

        if(phase == newSamplePeriod){ // Even border, no fancy math
            array[samplesWritten] = carry;
            // printf("out %2d Round border %d carry\n", samplesWritten+1, carry);
            carry = 0;
            phase = 0;
            samplesWritten++;
        }
        else if(phase > newSamplePeriod){ // Previous interval cut in some fraction, divide apropriately
            uint16_t fractionToCarryOver = phase - newSamplePeriod;
            uint32_t tmp = prevValue * fractionToCarryOver;
            uint16_t carryToNext = tmp / prevSamplePeriod; // Main fraction
            if(tmp - carryToNext * prevSamplePeriod > prevSamplePeriod / 2) // Rounding
                carryToNext++;

            carry -= carryToNext; // Take off the part that will be carried over
            array[samplesWritten] = carry; // Write the current sample
            phase = fractionToCarryOver; // Adjust the phase
            // printf("out %2d Partial border (%d/%d) %d stored, %d carry over\n", samplesWritten+1, phase, newSamplePeriod, carry, carryToNext);
            carry = carryToNext; // Carry over the remainder
            samplesWritten++;
        }

        if(samplesWritten > i){
            printf("Overrun! i: %d, samplesWritten: %d\n", i, samplesWritten);
            return samplesWritten;
        }
    }

    return samplesWritten;
}

uint32_t upsampleDiscreteArray(uint16_t *array, uint16_t arrLen, uint16_t prevSamplePeriod, uint16_t newSamplePeriod){
    uint32_t newArrayDuration = arrLen * newSamplePeriod;
    uint32_t startPoint = newArrayDuration / prevSamplePeriod; // Might need +1
    int32_t currentAddress = arrLen - 1;
    int32_t phase = (newArrayDuration % prevSamplePeriod);
    int32_t carry = 0;

    if(phase) // Remove the fraction left "out of the window"
        carry = ((array[startPoint] * phase) / prevSamplePeriod) - array[startPoint];

    // Upsampling starts from the oldest sample
    for(int32_t i = startPoint; (i >= 0 ) && (currentAddress >= 0); i--){
        uint32_t origValue = array[i];
        carry += origValue;

        printf("in %2d - Phase: %d, Value: %d, Carry: %d\n", i, phase, origValue, carry);

        do{
            phase -= newSamplePeriod;

            if(phase > 0){
                uint32_t newSampleValue = (origValue * newSamplePeriod) / prevSamplePeriod;

                array[currentAddress] = newSampleValue;
                printf("out %2d mid-phase %d, val %d, %d carry\n", currentAddress, phase, array[currentAddress], carry);
                carry -= newSampleValue;
                currentAddress--;
            }
            else{ // phase < 0 or phase == 0
                int32_t fractionFromNextSample = 0;

                if(phase < 0 && i > 0){
                    fractionFromNextSample = (((int32_t)array[i-1]) * -phase) / (int32_t)newSamplePeriod;
                    printf("Fraction from next %d, carry %d\n", fractionFromNextSample, carry);
                }
                array[currentAddress] = carry + fractionFromNextSample;
                printf("out %2d end-phase %d, val %d, %d carry\n", currentAddress, phase, array[currentAddress], carry);
                carry = -fractionFromNextSample; // Remove the "borrowed" portion from the next sample
                currentAddress--;
            }
        }while(phase > 0);

        phase += prevSamplePeriod;
    }

    return (arrLen - 1) - currentAddress;
}

uint32_t resampleDiscreteArray(uint16_t *array, uint16_t arrLen, uint16_t prevSamplePeriod, uint16_t newSamplePeriod){
    if(prevSamplePeriod % newSamplePeriod && newSamplePeriod % prevSamplePeriod){
        // Factorize the numbers
        uint16_t lcd = lowestCommonDenominator(prevSamplePeriod, newSamplePeriod);
        uint16_t tmp = prevSamplePeriod;
        prevSamplePeriod = lcd / newSamplePeriod;
        newSamplePeriod = lcd / tmp;
    }

    if(prevSamplePeriod < newSamplePeriod)
        return downsampleDiscreteArray(array, arrLen, prevSamplePeriod, newSamplePeriod);
    else if(prevSamplePeriod > newSamplePeriod)
        return upsampleDiscreteArray(array, arrLen, prevSamplePeriod, newSamplePeriod);

    return arrLen;
}


int main(void){
    // uint16_t exampleArrayOrig[] = {1,2,3,4,5,6,7,8,9,10};
    // uint16_t exampleArray[] = {1,2,3,4,5,6,7,8,9,10};
    uint16_t exampleArray[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    uint16_t exampleArrayOrig[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    uint16_t exampleArrayExpectedOutput1_5[] = {15, 40};
    uint16_t exampleArrayExpectedOutput2_5[] = {5,10,17,23};
    uint16_t exampleArrayExpectedOutput3_5[] = {2,5,8,11,13,16};
    uint16_t exampleArrayExpectedOutput4_5[] = {2,3,4,6,8,9,11,12};

    for(uint16_t interval = 600; interval <= 20*60; interval += 200){
        memcpy(exampleArray, exampleArrayOrig, sizeof(exampleArrayOrig));
        uint16_t oldArrLen = sizeof(exampleArray)/sizeof(exampleArray[0]);
        uint16_t oldInterval = interval;
        uint16_t newInterval = 5*60;
        uint16_t newSamples = resampleDiscreteArray(exampleArray, oldArrLen, oldInterval, newInterval);
        // printf("%d -> %d samples\n", oldArrLen, newSamples);

        uint64_t origSum = 0;
        for(uint16_t i = 0; i < oldArrLen; i++){
            origSum += exampleArrayOrig[i];
        }

        uint64_t newSum = 0;
        for(uint16_t i = 0; i < newSamples; i++){
            // printf("%2d - %d \n", i, exampleArray[i]);
            newSum += exampleArray[i];
        }

        if(origSum == newSum)
            printf("✅ %d Sum match. ", interval);
        else
            printf("❌ %d !!!!! Sum mismatch !!!!! ", interval);
        printf(" (orig: %d (%d samp) new: %d (%d samp))\n", origSum, oldArrLen, newSum, newSamples);

    }
}