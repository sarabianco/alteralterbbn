#include "include.h"

// An arrray to store info about the cosmological evolution
double **cosmo_array;
int COSMO_ROWS;

// The logarithmic spacing between two neighboring points in T
double delta_logx_cosmo;

// A flag to determine if a cosmo_file was loaded
bool cosmo_file_loaded = false;


SortOrder determine_sort_order(double *arr, int size) {
    if (size < 2) return ASCENDING;

    return arr[0] < arr[1] ? ASCENDING : DESCENDING;
}


int find_index(double *arr, int size, double x) {
    if (size < 2) return -1; // Cannot determine position for arrays smaller than 2

    SortOrder order = determine_sort_order(arr, size);
    
    int left = 0;
    int right = size - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if ((order == ASCENDING && arr[mid] <= x && (mid == size - 1 || arr[mid + 1] > x)) ||
            (order == DESCENDING && arr[mid] >= x && (mid == size - 1 || arr[mid + 1] < x))) {
            return mid;
        }
        
        if ((order == ASCENDING && arr[mid] < x) || (order == DESCENDING && arr[mid] > x)) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return -1;
}


void read_cosmo_file(char *filename, int nrows) {
    FILE* f = fopen(filename, "r");

    if ( f == NULL ) {
        perror("Could not open the provided cosmo-file:");
        
        exit(1);
    }

    COSMO_ROWS = nrows;

    cosmo_array = malloc(COSMO_COLS * sizeof(double*));
    for (int col = 0; col < COSMO_COLS; col++ ) {
        cosmo_array[col] = malloc(COSMO_ROWS * sizeof(double));
    }

    for( int row = 0; row < COSMO_ROWS; row++ ) {
        fscanf(f, "%lf %lf %lf %lf %lf %lf %lf", &cosmo_array[0][row], &cosmo_array[1][row], &cosmo_array[2][row], &cosmo_array[3][row], &cosmo_array[4][row], &cosmo_array[5][row], &cosmo_array[6][row]);
    }

    delta_logx_cosmo = log(cosmo_array[COSMO_COL_t][1]/cosmo_array[COSMO_COL_t][0]);

    fclose(f);

    cosmo_file_loaded = true;
}


double interp_cosmo_array(int i_col, double x) {
    if ( !cosmo_file_loaded ) {
        fprintf(stderr, "ERROR: Cannot interpolate data, since no cosmo-file has been loaded");

        exit(1);
    }

    double r = (log(x) - log(cosmo_array[COSMO_COL_t][0]))/delta_logx_cosmo;
    int r_div = (int)floor(r);
    double r_mod = r - r_div;

    if ( r_div < 0 || r_div > COSMO_ROWS - 2 ) {
        fprintf(stderr, "Cannot interpolate data: index out of range\n");

        exit(1);
    }

    return cosmo_array[i_col][r_div] * (1 - r_mod) +  cosmo_array[i_col][r_div+1]*r_mod;
}


double cosmo_t_T(double T) {
    if ( !cosmo_file_loaded ) {
        fprintf(stderr, "ERROR: Cannot calculate t(T) since no cosmo-file has been loaded\n");

        exit(1);
    }

    double logt_min = log(cosmo_array[COSMO_COL_t][0]);
    double logt_max = log(cosmo_array[COSMO_COL_t][COSMO_ROWS-1]);

    double eps = 1e-5;
    int N_itermax = 100;
    double logt_tmp, T_tmp;

    for( int i = 0; i < N_itermax; i++ ) {
        logt_tmp = 0.5*(logt_min + logt_max);
        T_tmp = interp_cosmo_array(COSMO_COL_T, exp(logt_tmp));
        if ( fabs((T - T_tmp)/T) < eps ) {
            return exp(logt_tmp);
        }
        if ( T_tmp > T ) {
            logt_min = logt_tmp;
        } else {
            logt_max = logt_tmp;
        }
    }

    fprintf(stderr, "ERROR: The bisection method to determine t(T) does not converge\n");
    
    exit(1);

    return 0.;
}
