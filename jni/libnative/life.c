#define F(n)for(n=1;n<33;n++)

#define l(a) + !(2 * a & x)+!( a & x)+!(a >> 1 & x)

int Q[68];
int *E=Q;
int *R=Q+34;
int y;
int i;

    main(){
        srand(time(0));
        for (i=1; i<33; i++) E[i]=rand()<<8;
        B:
        for (y=1; y<33; y++) {
            R[y]=0;
            for (i=1; i<33; i++) {
                int x = 1 << (i-1);
                int A = l(E[y-1]) l(E[y]) l(E[y+1]);
                q = A == 6 || E[y] & x && A == 5;
                R[y] |= q ? x : 0;
                printf(q?"[]":"  ");}
                puts("");
            }
        }
        i=E; E=R; R=i;
        system("sleep .1;clear");
        goto B;
    }
