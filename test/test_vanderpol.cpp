#include "basic.hpp"
#include <catch2/catch_test_macros.hpp>

int VanderPol()
{
    constexpr int num_states = 2;
    constexpr int num_output = 2;
    constexpr int num_inputs = 1;
    constexpr int pred_hor = 10;
    constexpr int ctrl_hor = 5;
    constexpr int ineq_c = pred_hor + 1;
    constexpr int eq_c = 0;

    double ts = 0.1;

#ifdef MPC_DYNAMIC
    mpc::NLMPC<> optsolver(
        num_states, num_inputs, num_output,
        pred_hor, ctrl_hor,
        ineq_c, eq_c);
#else
    mpc::NLMPC<
        num_states, num_inputs, num_output,
        pred_hor, ctrl_hor,
        ineq_c, eq_c>
        optsolver;
#endif

    optsolver.setLoggerLevel(mpc::Logger::log_level::NORMAL);
    optsolver.setContinuosTimeModel(ts);

    auto stateEq = [&](
                       mpc::cvec<TVAR(num_states)>& dx,
                       mpc::cvec<TVAR(num_states)> x,
                       mpc::cvec<TVAR(num_inputs)> u) {
        dx(0) = ((1.0 - (x(1) * x(1))) * x(0)) - x(1) + u(0);
        dx(1) = x(0);
    };

    optsolver.setStateSpaceFunction(stateEq);

    optsolver.setObjectiveFunction([&](
                                       mpc::mat<TVAR(pred_hor + 1), TVAR(num_states)> x,
                                       mpc::mat<TVAR(pred_hor + 1), TVAR(num_inputs)> u,
                                       double) {
        return x.array().square().sum() + u.array().square().sum();
    });

    optsolver.setIneqConFunction([&](
                                     mpc::cvec<TVAR(ineq_c)>& in_con,
                                     mpc::mat<TVAR(pred_hor + 1), TVAR(num_states)>,
                                     mpc::mat<TVAR(pred_hor + 1), TVAR(num_output)>,
                                     mpc::mat<TVAR(pred_hor + 1), TVAR(num_inputs)> u,
                                     double) {
        for (int i = 0; i < ineq_c; i++) {
            in_con(i) = u(i, 0) - 0.5;
        }
    });

    mpc::cvec<TVAR(num_states)> modelX, modeldX;
    modelX.resize(num_states);
    modeldX.resize(num_states);

    modelX(0) = 0;
    modelX(1) = 1.0;

    auto r = optsolver.getLastResult();

    for (;;) {
        r = optsolver.step(modelX, r.cmd);
        auto seq = optsolver.getOptimalSequence();
        stateEq(modeldX, modelX, r.cmd);
        modelX += modeldX * ts;
        if (std::fabs(modelX[0]) <= 1e-2 && std::fabs(modelX[1]) <= 1e-1) {
            break;
        }
    }

    return 0;
}

TEST_CASE(
    MPC_TEST_NAME("Vanderpol example"),
    MPC_TEST_TAGS("[vanderpol]"))
{
    REQUIRE(VanderPol() == 0);
}