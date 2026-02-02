#include <stdio.h>

int mylicm(int n){
    int sum=0;
    int a=3;
    int b=2;

    for (int i=0; i<n; i++){
        int v = a+b;
        sum+=v;

    }

    return sum;
}

int main(){
    int risultato=mylicm(5);
    printf("risultato=%d\n",risultato);
    return 0;
}