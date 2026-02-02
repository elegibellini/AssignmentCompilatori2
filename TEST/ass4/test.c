#include <stdio.h>

int main(){
    int N=10;
    int A[N];
    int B[N];

    for(int i=0; i<N; i++){
        A[i]=i;
    }

    for(int i=0; i<N; i++){
        B[i]=A[i]+2;
    }
}