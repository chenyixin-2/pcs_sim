#ifndef __POWER_H__
#define __POWER_H__

struct power_t{
    int ap; // kW
    int aps;
    int last_aps;
    //double Pd_limit; //
    //double Pd_rate;  //
    //double Pd_max;   //
    //double Pc_limit;
    //double Pc_rate;
    //double Pc_max;

    //double P_r;
    //double lastP_r;
    //double Pset;
    //double lastPset;
    //double orgP_r;

    int norm_cap; // kWh
    int norm_pow; // kW
    int min_pow; // kW
    double soc; // 0 - 100
    double soh; // 0 - 100
    //double socc;
    //double socd;
    int bdhgable;
    int bchgable;
};

#endif