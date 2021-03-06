#include "Backends/Cubics/CubicBackend.h"

class PureFluid : public CoolProp::PengRobinsonBackend{
public:
    PureFluid(const std::vector<double> &Tc,
              const std::vector<double> &pc,
              const std::vector<double> &acentric,
              double R_u,
              bool generate_SatL_and_SatV = true)
        : PengRobinsonBackend(Tc,pc,acentric,R_u, generate_SatL_and_SatV)
    { };
    void set_c_MC(const std::vector<double> &c123){
        // set constants c1, c2, c3 for Mathias-Copeman
        AbstractCubicBackend::set_C_MC(c123[0], c123[1], c123[2]);
    }
    void set_c_Twu(const std::vector<double> &c123){
        // set constants c1, c2, c3 for Twu
        AbstractCubicBackend::set_C_Twu(c123[0], c123[1], c123[2]);
    }
    double saturation_pressure(double T){
        update(CoolProp::QT_INPUTS, 0, T);
        return p();
    }
    double saturation_temp(double p){
        update(CoolProp::PQ_INPUTS, p, 0);
        return T();
    }
    double rhomolar() {
        return AbstractState::rhomolar();
    }
    double am(double T) { 
        double tau = get_cubic()->T_r/T;
        std::vector<double> x(1, 1.0);
        return get_cubic()->am_term(tau, x, 0);
    }
    double bm() { 
        std::vector<double> x(1, 1.0); 
        return get_cubic()->bm_term(x); 
    }
    double Delta_1() { return get_cubic()->get_Delta_1(); }
    double Delta_2() { return get_cubic()->get_Delta_2(); }
};

#ifdef PYBIND11

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

PYBIND11_PLUGIN(PureFluid) {
    py::module m("PureFluid", "Pure fluid plugin");
    py::class_<PureFluid>(m, "PureFluid")
        .def(py::init<const std::vector<double> &, const std::vector<double> &, const std::vector<double> &, double, bool>())
        .def("set_c_MC", &PureFluid::set_c_MC)
        .def("set_c_Twu", &PureFluid::set_c_Twu)
        .def("saturation_pressure", &PureFluid::saturation_pressure)
        .def("saturation_temp", &PureFluid::saturation_temp)
        .def("rhomolar", &PureFluid::rhomolar)
        .def("am", &PureFluid::am)
        .def("bm", &PureFluid::bm)
        .def("Delta_1", &PureFluid::Delta_1)
        .def("Delta_2", &PureFluid::Delta_2)
        ;
    return m.ptr();
}

#else 
int main() {
    #include "crossplatform_shared_ptr.h"
    #include "AbstractState.h"
    using namespace CoolProp;
    shared_ptr<AbstractState> c2(AbstractState::factory("HEOS", "ethane")); 
    PureFluid pf({ c2->T_critical() }, { c2->p_critical() }, { c2->acentric_factor() }, c2->gas_constant(), true);
    for (double T = 300; T < c2->T_critical(); T += 0.1) {
        std::string rpfluid = "REFPROP::ethane";
        double rhoL = PropsSI("Dmolar", "T", T, "Q", 0, rpfluid);
        double rhoV = PropsSI("Dmolar", "T", T, "Q", 1, rpfluid);
        double pppp = PropsSI("P", "T", T, "Q", 1, rpfluid);
        c2->update(QT_INPUTS, 0, T);
        
        double pEth = c2->p(); 
        double pSRK = pf.saturation_pressure(T);
        try {
            pSRK = pf.saturation_pressure(T);
            double rhoL = pf.rhomolar();
        }
        catch (...) {
            pf.saturation_pressure(T);
        }
    }
    int rr = 4;
}
#endif