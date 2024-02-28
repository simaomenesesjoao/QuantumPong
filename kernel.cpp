#define prod(a,b)(float2)(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x)

void kernel set_sq(__global float2 *hops, __global float *SCALE){

    //int L = LX + 2*PAD;
    //int j = (get_global_id(1)-PAD)*L + (get_global_id(0)-PAD);
    int i = get_global_id(1)*LX + get_global_id(0);
    int j5 = NHOPS*i;
    float t = -1.0/SCALE[0];

    hops[j5+0] = (float2)(0.0, 0.0); // Local potential
    hops[j5+1] = (float2)(  t, 0.0); // from the left
    hops[j5+2] = (float2)(  t, 0.0); // from the right
    hops[j5+3] = (float2)(  t, 0.0); // from up
    hops[j5+4] = (float2)(  t, 0.0); // from down

    //if(get_global_id(1)==100)
        //hops[j5+3] = (float2)(0.0, 0.0); // from up
}


void kernel set_local_pot(__global float2 *hops, __global float *SCALE, __global float *val){

    int i = get_global_id(1)*LX + get_global_id(0);
    int j5 = NHOPS*i;
    float t = val[0]/SCALE[0];

    hops[j5+0] = (float2)(t, 0.0); // Local potential
}

void kernel clear_local_pot(__global float2 *hops){
    int i = get_global_id(1)*LX + get_global_id(0);
    int j5 = NHOPS*i;
    hops[j5] = (float2)(0.0, 0.0); // Local potential
}

void kernel cheb2(__global float2 *input, __global float2 *output, __global float2 *hops, __global float2 *acc, __global float2 *jn){


    int L = LX + 2*PAD;
    int j = (get_global_id(1)-PAD)*LX + (get_global_id(0)-PAD);
    int i = get_global_id(1)*L + get_global_id(0);
    int j4 = NHOPS*j;

    float2 sum = (float2)(0.0, 0.0);
    sum += prod(input[i],  hops[j4]);
    sum += prod(input[i-1],hops[j4+1]);
    sum += prod(input[i+1],hops[j4+2]);
    sum += prod(input[i-L],hops[j4+3]);
    sum += prod(input[i+L],hops[j4+4]);

    output[i] = 2*sum - output[i]; 
    acc[i] += prod(output[i],jn[0]);
}

void kernel cheb1(__global float2 *input, __global float2 *output, __global float2 *hops, __global float2 *acc, __global float2 *j0, __global float2 *j1){


    int L = LX + 2*PAD;
    int j = (get_global_id(1)-PAD)*LX + (get_global_id(0)-PAD);
    int i = get_global_id(1)*L + get_global_id(0);
    int j4 = NHOPS*j;

    //printf("%d %d %d %d %f %f %f %f %f %f %f %f %f %f\n",get_global_id(0), get_global_id(1), i,j,input[i], input[i-1], input[i+1], input[i+L], input[i-L], hops[j4], hops[j4+1], hops[j4+2], hops[j4+3], hops[j4+4]);

    float2 sum = (float2)(0.0, 0.0);
    sum += prod(input[i],  hops[j4]);
    sum += prod(input[i-1],hops[j4+1]);
    sum += prod(input[i+1],hops[j4+2]);
    sum += prod(input[i-L],hops[j4+3]);
    sum += prod(input[i+L],hops[j4+4]);
    output[i] = sum;

    acc[i] += prod(input[i],j0[0]);
    acc[i] += prod(output[i],j1[0]);
}


void kernel colormap(__global float2 *acc, __global int4 *pix, __global float *max){
    float m = max[0];
    int L = LX + 2*PAD;
    int j = (get_global_id(1)-PAD)*LX + (get_global_id(0)-PAD);
    int i = get_global_id(1)*L + get_global_id(0);

    //int value = (int)(255*(acc[i].x/max));
    int value = (int)(255*(acc[i].x*acc[i].x + acc[i].y*acc[i].y)/m);

    //printf("max %f\n", m);
    //printf("%d %d %d %d %d\n", get_global_id(0) ,get_global_id(1), i, j, value);
    //printf("%d %d %d %d %f %f %d\n", get_global_id(0) ,get_global_id(1), i, j, acc[i].x, acc[i].y, value);
    //int value = 0;
    pix[j].x = value/2; // B
    pix[j].y = value;   // G
    pix[j].z = value/3; // R
    pix[j].w = 0;       // A
}

void kernel absorb(__global float2 *array, __global float *score){
    int L = LX + 2*PAD;
    int y = get_global_id(1);
    int i = y*L + get_global_id(0);

    float dif = 0.95;
    float re = array[i].x;
    float im = array[i].y;

    // p = p*dif
    // dp = p-p*dif = (1-dif)p
    score[i] += (1-dif*dif)*(re*re + im*im);
    array[i].x = dif*re;
    array[i].y = dif*im;
}


void kernel copy(__global float2 *from, __global float2 *to){
    int L = LX + 2*PAD;
    int i = get_global_id(1)*L + get_global_id(0);
    to[i] = from[i];
}

void kernel reset(__global float2 *array){
    int L = LX + 2*PAD;
    int i = get_global_id(1)*L + get_global_id(0);
    array[i].x = 0;
    array[i].y = 0;
}
