#define prod(a,b)(float2)(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x)

void kernel set_sq(__global float2 *hops, __global float *scale){

    //int l = lx + 2*pad;
    //int j = (get_global_id(1)-pad)*l + (get_global_id(0)-pad);
    int i = get_global_id(1)*LX + get_global_id(0);
    int j5 = NHOPS*i;
    float t = -1.0/scale[0];

    hops[j5+0] = (float2)(0.0, 0.0); // local potential
    hops[j5+1] = (float2)(  t, 0.0); // from the left
    hops[j5+2] = (float2)(  t, 0.0); // from the right
    hops[j5+3] = (float2)(  t, 0.0); // from up
    hops[j5+4] = (float2)(  t, 0.0); // from down

}

void kernel set_sq_B(__global float2 *hops, __global float *SCALE, __global float *B, __global float *V){

    int x = get_global_id(0);
    int y = get_global_id(1);

    int i = y*LX + x;
    int i5 = NHOPS*i;
    int L = LX+2*PAD;
    int j = (y+PAD)*L + (x+PAD);

    float t = -1.0/SCALE[0];

    float arg1 = 0.5*(B[j]+B[j-1])*y;
    float arg2 = 0.5*(B[j]+B[j+1])*y;
    float localV = V[i]/SCALE[0];
    //if( B[j-1]*B[j-1] > 0.1)
        //printf("B: %f %f %f", B[j], B[j-1], B[j+1]);

    //float arg1 = 0;
    //float arg2 = 0;
    //float localV = 0;

    float2 peierls1 = (float2)(t*cos(arg1),  t*sin(arg1));
    float2 peierls2 = (float2)(t*cos(arg2), -t*sin(arg2));

    hops[i5+0] = (float2)(localV, 0.0); // Local potential
    hops[i5+1] = peierls1;
    hops[i5+2] = peierls2;
    hops[i5+3] = (float2)(  t, 0.0); // from up
    hops[i5+4] = (float2)(  t, 0.0); // from down

}


void kernel set_local_pot(__global float *local_pot, __global float *val, __global bool *changed){

    int x = get_global_id(0);
    int y = get_global_id(1);
    int i = y*LX + x;
    //int j5 = NHOPS*i;

    // Get the radius, assumes that the global sizes have the same dimension
    int rad = get_global_size(0)/2;

    int x0 = get_global_offset(0);
    int y0 = get_global_offset(1);
    int dx = x-x0-rad;
    int dy = y-y0-rad;
    float dif=0;

    if(x>=0 && x < LX && y>=0 && y<LY){
        if(dx*dx + dy*dy < rad*rad){
            dif = local_pot[i]-val[0];

            // Many threads write to this at the same time. It should be fine though
            if(dif*dif > 1e-5) changed[0] = true;

            local_pot[i] = val[0];
        }
    }

}

void kernel set_local_pot_rect(__global float2 *hops, __global float *pot, __global float *val, __global float *SCALE, __global bool *changed){

    int x = get_global_id(0);
    int y = get_global_id(1);
    int i = y*LX + x;
    int j5 = NHOPS*i;

    float v = val[0]/SCALE[0];
    hops[j5] = (float2)(v, 0.0); // Local potential

    float dif = pot[i] - val[0];
    if(dif*dif > 1e-5) changed[0] = true;


    pot[i] = val[0];

}

void kernel set_local_B(__global float *mag, __global float *val){

    int x = get_global_id(0);
    int y = get_global_id(1);
    int L = LX + 2*PAD;
    int i = y*L + x;

    // Get the radius, assumes that the global sizes have the same dimension
    int rad = get_global_size(0)/2;

    int x0 = get_global_offset(0);
    int y0 = get_global_offset(1);
    int dx = x-x0-rad;
    int dy = y-y0-rad;

    float frac = (dx*dx + dy*dy)*1.0/(rad*rad);
    //float attenuation = exp(-frac*frac*2);
    float attenuation = exp(-frac*2);
    float dm = val[0]*attenuation;
    //attenuation = 1;
    float max_mag = val[0]*4;


    if(x>=0 && x < LX && y>=0 && y<LY && frac < 1){
        if(mag[i]+dm > max_mag){
            mag[i] = max_mag;
        } else {
            mag[i] += dm;
        }
    }

}

void kernel clear_local_pot(__global float2 *hops, __global float *pot, __global bool *changed){
    int i = get_global_id(1)*LX + get_global_id(0);
    int j5 = NHOPS*i;
    // printf("%d %d %d %d d%d\n", get_global_id(0), get_global_id(1), i, j5, NHOPS );
    hops[j5] = (float2)(0.0, 0.0); // Local potential

    float dif = pot[i] - 0.0;
    if(dif*dif > 1e-5) changed[0] = true;
    pot[i] = 0;
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
    int value = (int)(255*(acc[i].x*acc[i].x + acc[i].y*acc[i].y)/m/m);
    int valueo2 = value/2;
    int valueo3 = value/3;
    if(value>255) value=255;
    if(valueo2>255) valueo2=255;
    if(valueo3>255) valueo3=255;

    //printf("max %f\n", m);
    //printf("%d %d %d %d %d\n", get_global_id(0) ,get_global_id(1), i, j, value);
    //printf("%d %d %d %d %f %f %d\n", get_global_id(0) ,get_global_id(1), i, j, acc[i].x, acc[i].y, value);
    //int value = 0;
    pix[j].x = valueo2; // B
    pix[j].y = value;   // G
    pix[j].z = valueo3; // R
    pix[j].w = 0;       // A
}

void kernel colormapV(__global float *pot, __global int4 *pix){
    int i = get_global_id(1)*LX + get_global_id(0);
    float max=2.0;
    //float max=8.0; // actual value
    int value = (int)(255*pot[i]/max);

    //printf("max %f\n", m);
    //printf("%d %d %d %d %d\n", get_global_id(0) ,get_global_id(1), i, j, value);
    //printf("%d %d %d %d %f %f %d\n", get_global_id(0) ,get_global_id(1), i, j, acc[i].x, acc[i].y, value);
    //int value = 0;
    pix[i].x = value; // B
    pix[i].y = 0;   // G
    pix[i].z = 0; // R
    pix[i].w = 0;       // A
}



void kernel colormapB(__global float *mag, __global int4 *pix){
    int L = LX + 2*PAD;
    int x = get_global_id(0);
    int y = get_global_id(1);

    int i = y*LX + x;
    int j = (y+PAD)*L + (x+PAD);

    float max=0.005*4*2;
    int value = (int)(255*mag[j]/max);

    pix[i].x = 0; // B
    pix[i].y = 0;   // G
    pix[i].z = value; // R
    pix[i].w = 0;       // A
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
