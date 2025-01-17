#pragma once

#include <mpc/IMPC.hpp>
#include <mpc/LMPC/LOptimizer.hpp>
#include <mpc/LMPC/ProblemBuilder.hpp>

namespace mpc
{
    /**
     * @brief Linear MPC front-end class
     *
     * @tparam Tnx dimension of the state space
     * @tparam Tnu dimension of the input space
     * @tparam Tndu dimension of the measured disturbance space
     * @tparam Tny dimension of the output space
     * @tparam Tph length of the prediction horizon
     * @tparam Tch length of the control horizon
     */
    template <
        int Tnx = Eigen::Dynamic, int Tnu = Eigen::Dynamic, int Tndu = Eigen::Dynamic,
        int Tny = Eigen::Dynamic, int Tph = Eigen::Dynamic, int Tch = Eigen::Dynamic>
    class LMPC : public IMPC<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>
    {

    private:
        using IMPC<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::optPtr;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::setDimension;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::nu;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::nx;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::ndu;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::ny;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::ph;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::ch;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::ineq;
        using IDimensionable<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::eq;

    public:
        using IMPC<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::step;
        using IMPC<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::setLoggerLevel;
        using IMPC<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::setLoggerPrefix;
        using IMPC<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>::getLastResult;

    public:
        LMPC()
        {
            setDimension();
        }

        LMPC(
            const int &nx, const int &nu, const int &ndu,
            const int &ny, const int &ph, const int &ch)
        {
            setDimension(nx, nu, ndu, ny, ph, ch);
        }

        ~LMPC() = default;

        /**
         * @brief (NOT AVAILABLE) Set the discretization time step to use for numerical integration
         */
        bool setContinuosTimeModel(const double /*ts*/)
        {
            throw std::runtime_error("Linear MPC supports only discrete time systems");
            return false;
        }

        /**
         * @brief  Set the solver specific parameters
         *
         * @param param desired parameters (the structure must be of type LParameters)
         */
        void setOptimizerParameters(const Parameters &param)
        {
            ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setParameters(param);
        }

        /**
         * @brief (NOT AVAILABLE) Set the scaling factor for the control input
         *
         */
        void setInputScale(const cvec<Tnu> /*scaling*/)
        {
            throw std::runtime_error("Linear MPC does not support input scaling");
        }

        /**
         * @brief (NOT AVAILABLE) Set the scaling factor for the dynamical system's states variables
         *
         */
        void setStateScale(const cvec<Tnx> /*scaling*/)
        {
            throw std::runtime_error("Linear MPC does not support state scaling");
        }

        /**
         * @brief Set the state, input and output box constraints
         *
         * @param XMinMat minimum state matrix
         * @param UMinMat minimum input matrix
         * @param YMinMat minimum output matrix
         * @param XMaxMat maximum state matrix
         * @param UMaxMat maximum input matrix
         * @param YMaxMat maximum output matrix
         * @return true
         * @return false
         */
        bool setConstraints(
            const mat<Tnx, Tph> XMinMat, const mat<Tnu, Tph> UMinMat, const mat<Tny, Tph> YMinMat,
            const mat<Tnx, Tph> XMaxMat, const mat<Tnu, Tph> UMaxMat, const mat<Tny, Tph> YMaxMat)
        {
            Logger::instance().log(Logger::log_type::DETAIL) << "Setting constraints" << std::endl;
            return builder.setConstraints(
                XMinMat, UMinMat, YMinMat,
                XMaxMat, UMaxMat, YMaxMat);
        }

        /**
         * @brief Set the state, input and output box constraints, the constraints are applied equally
         * along the specified prediction horizon segment
         *
         * @param XMin minimum state vector
         * @param UMin minimum input vector
         * @param YMin minimum output vector
         * @param XMax maximum state vector
         * @param UMax maximum input vector
         * @param YMax maximum output vector
         * @param slice slice of the prediction horizon step [start end]
         * (if both ends re set to -1 the whole prediction horizon is used)
         * @return true
         * @return false
         */
        bool setConstraints(
            const cvec<Tnx> XMin, const cvec<Tnu> UMin, const cvec<Tny> YMin,
            const cvec<Tnx> XMax, const cvec<Tnu> UMax, const cvec<Tny> YMax,
            const std::array<int, 2> slice)
        {
            size_t start = slice[0];
            size_t end = slice[1];

            if (start >= end || start > ph() || end > ph() || start + end > ph())
            {
                Logger::instance().log(Logger::log_type::ERROR) << "The horizon slice is out of bounds" << std::endl;
                return false;
            }

            if (start == -1 && end == -1)
            {
                // replicate all along the prediction horizon
                mat<Tnx, Tph> XMinMat, XMaxMat;
                mat<Tny, Tph> YMinMat, YMaxMat;
                mat<Tnu, Tph> UMinMat, UMaxMat;

                XMinMat.resize(nx(), ph());
                YMinMat.resize(ny(), ph());
                UMinMat.resize(nu(), ph());

                XMaxMat.resize(nx(), ph());
                YMaxMat.resize(ny(), ph());
                UMaxMat.resize(nu(), ph());

                for (size_t i = 0; i < ph(); i++)
                {
                    XMinMat.col(i) = XMin;
                    XMaxMat.col(i) = XMax;
                    YMinMat.col(i) = YMin;
                    YMaxMat.col(i) = YMax;

                    if (i < ph())
                    {
                        UMinMat.col(i) = UMin;
                        UMaxMat.col(i) = UMax;
                    }
                }

                Logger::instance().log(Logger::log_type::DETAIL) << "Setting constraints equally on the horizon" << std::endl;
                return builder.setConstraints(
                    XMinMat, UMinMat, YMinMat,
                    XMaxMat, UMaxMat, YMaxMat);
            }
            else
            {
                bool ret = true;

                for (size_t i = start; i < end; i++)
                {
                    Logger::instance().log(Logger::log_type::DETAIL) << "Setting constraints for the step " << i << std::endl;
                    ret = ret && builder.setConstraints(i, XMin, UMin, YMin, XMax, UMax, YMax);
                }

                return ret;
            }
        }

        /**
         * @brief Set the objective function weights, the weights are applied equally
         * along the specified prediction horizon segment
         *
         * @param OWeight weights for the output vector
         * @param UWeight weights for the optimal control input vector
         * @param DeltaUWeight weight for the variation of the optimal control input vector
         * @param slice slice of the prediction horizon step [start end]
         * (if both ends re set to -1 the whole prediction horizon is used)
         * @return true
         * @return false
         */
        bool setObjectiveWeights(
            const mat<Tny, Tph> &OWeightMat,
            const mat<Tnu, Tph> &UWeightMat,
            const mat<Tnu, Tph> &DeltaUWeightMat)
        {
            Logger::instance().log(Logger::log_type::DETAIL) << "Setting weights" << std::endl;
            return builder.setObjective(OWeightMat, UWeightMat, DeltaUWeightMat);
        }

        /**
         * @brief Set the objective function weights, the weights are applied equally
         * along the specified prediction horizon segment
         *
         * @param OWeight weights for the output vector
         * @param UWeight weights for the optimal control input vector
         * @param DeltaUWeight weight for the variation of the optimal control input vector
         * @param slice slice of the prediction horizon step where to apply the constraints [start end]
         * (if both ends re set to -1 the whole prediction horizon is used)
         * @return true
         * @return false
         */
        bool setObjectiveWeights(
            const cvec<Tny> &OWeight,
            const cvec<Tnu> &UWeight,
            const cvec<Tnu> &DeltaUWeight,
            const std::array<int, 2> slice)
        {
            size_t start = slice[0];
            size_t end = slice[1];

            if (start >= end || start > ph() || end > ph() || start + end > ph())
            {
                Logger::instance().log(Logger::log_type::ERROR) << "The horizon slice is out of bounds" << std::endl;
                return false;
            }

            if (start == -1 && end == -1)
            {
                // replicate all along the prediction horizon
                mat<Tny, Tph> OWeightMat;
                mat<Tnu, Tph> UWeightMat;
                mat<Tnu, Tph> DeltaUWeightMat;

                OWeightMat.resize(ny(), ph());
                UWeightMat.resize(nu(), ph());
                DeltaUWeightMat.resize(nu(), ph());

                for (size_t i = 0; i < ph(); i++)
                {
                    OWeightMat.col(i) = OWeight;
                    UWeightMat.col(i) = UWeight;
                    DeltaUWeightMat.col(i) = DeltaUWeight;
                }

                Logger::instance().log(Logger::log_type::DETAIL) << "Setting weights equally on the horizon" << std::endl;
                return builder.setObjective(OWeightMat, UWeightMat, DeltaUWeightMat);
            }
            else
            {
                bool ret = true;

                for (size_t i = start; i < end; i++)
                {
                    Logger::instance().log(Logger::log_type::DETAIL) << "Setting weights for the step " << i << std::endl;
                    ret = ret && builder.setObjective(i, OWeight, UWeight, DeltaUWeight);
                }

                return ret;
            }
        }

        /**
         * @brief Set the state space model matrices
         * x(k+1) = A*x(k) + B*u(k) + Bd*d(k)
         * y(k) = C*x(k) + Dd*d(k)
         * @param A state update matrix
         * @param B input matrix
         * @param C output matrix
         * @return true
         * @return false
         */
        bool setStateSpaceModel(
            const mat<Tnx, Tnx> &A, const mat<Tnx, Tnu> &B,
            const mat<Tny, Tnx> &C)
        {

            Logger::instance().log(Logger::log_type::DETAIL) << "Setting state space model" << std::endl;
            return builder.setStateModel(A, B, C);
        }

        /**
         * @brief Set the disturbances matrices
         * x(k+1) = A*x(k) + B*u(k) + Bd*d(k)
         * y(k) = C*x(k) + Dd*d(k)
         * @param Bd state disturbance matrix
         * @param Dd output disturbance matrix
         * @return true
         * @return false
         */
        bool setDisturbances(
            const mat<Tnx, Tndu> &Bd,
            const mat<Tny, Tndu> &Dd)
        {

            Logger::instance().log(Logger::log_type::DETAIL) << "Setting disturbances matrices" << std::endl;
            return builder.setExogenuosInput(Bd, Dd);
        }

        /**
         * @brief Set the exogenuos inputs vector
         *
         * @param uMeas measured exogenuos input
         * @return true
         * @return false
         */
        bool setExogenuosInputs(
            const mat<Tndu, Tph> &uMeasMat)
        {
            return ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setExogenuosInputs(uMeasMat);
        }

        /**
         * @brief Set the exogenuos inputs vector, the exogenuos inputs are assumed to be constant
         * along the specified prediction horizon segment
         *
         * @param uMeas measured exogenuos input
         * @param slice slice of the prediction horizon [start end]
         * (if both ends re set to -1 the whole prediction horizon is used)
         * @return true
         * @return false
         */
        bool setExogenuosInputs(
            const cvec<Tndu> &uMeas,
            const std::array<int, 2> slice)
        {
            size_t start = slice[0];
            size_t end = slice[1];

            if (start >= end || start > ph() || end > ph() || start + end > ph())
            {
                Logger::instance().log(Logger::log_type::ERROR) << "The horizon slice is out of bounds" << std::endl;
                return false;
            }

            if (start == -1 && end == -1)
            {
                // replicate all along the prediction horizon
                mat<Tndu, Tph> uMeasMat;

                uMeasMat.resize(ndu(), ph());

                for (size_t i = 0; i < ph(); i++)
                {
                    uMeasMat.col(i) = uMeas;
                }

                return ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setExogenuosInputs(uMeasMat);
            }
            else
            {
                bool ret = true;

                for (size_t i = start; i < end; i++)
                {
                    ret = ret && ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setExogenuosInputs(i, uMeas);
                }

                return ret;
            }
        }

        /**
         * @brief Set the references matrix for the objective function
         *
         * @param outRef reference for the output
         * @param cmdRef reference for the optimal control input
         * @param deltaCmdRef reference for the variation of the optimal control input
         * @return true
         * @return false
         */
        bool setReferences(
            const mat<Tny, Tph> outRefMat,
            const mat<Tnu, Tph> cmdRefMat,
            const mat<Tnu, Tph> deltaCmdRefMat)
        {
            return ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setReferences(outRefMat, cmdRefMat, deltaCmdRefMat);
        }

        /**
         * @brief Set the references vector for the objective function, the references are assumed to be constant
         * along the specified prediction horizon segment
         *
         * @param outRef reference for the output
         * @param cmdRef reference for the optimal control input
         * @param deltaCmdRef reference for the variation of the optimal control input
         * @param slice slice of the prediction horizon step [start end]
         * (if both ends re set to -1 the whole prediction horizon is used)
         * @return true
         * @return false
         */
        bool setReferences(
            const cvec<Tny> outRef,
            const cvec<Tnu> cmdRef,
            const cvec<Tnu> deltaCmdRef,
            const std::array<int, 2> slice)
        {
            size_t start = slice[0];
            size_t end = slice[1];

            if (start >= end || start > ph() || end > ph() || start + end > ph())
            {
                Logger::instance().log(Logger::log_type::ERROR) << "The horizon slice is out of bounds" << std::endl;
                return false;
            }

            if (start == -1 && end == -1)
            {
                // replicate all along the prediction horizon
                mat<Tny, Tph> outRefMat;
                mat<Tnu, Tph> cmdRefMat;
                mat<Tnu, Tph> deltaCmdRefMat;

                outRefMat.resize(ny(), ph());
                cmdRefMat.resize(nu(), ph());
                deltaCmdRefMat.resize(nu(), ph());

                for (size_t i = 0; i < ph(); i++)
                {
                    outRefMat.col(i) = outRef;
                    cmdRefMat.col(i) = cmdRef;
                    deltaCmdRefMat.col(i) = deltaCmdRef;
                }

                return ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setReferences(outRefMat, cmdRefMat, deltaCmdRefMat);
            }
            else
            {
                bool ret = true;

                for (size_t i = start; i < end; i++)
                {
                    Logger::instance().log(Logger::log_type::DETAIL) << "Setting references for the step " << i << std::endl;
                    ret = ret && ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setReferences(i, outRef, cmdRef, deltaCmdRef);
                }

                return ret;
            }
        }

    protected:
        /**
         * @brief Initilization hook for the linear interface
         */
        void onSetup()
        {
            builder.initialize(
                nx(), nu(), ndu(), ny(),
                ph(), ch());

            optPtr = new LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)>();
            optPtr->initialize(
                nx(), nu(), ndu(), ny(),
                ph(), ch());

            ((LOptimizer<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> *)optPtr)->setBuilder(&builder);
        }

        /**
         * @brief (NOT AVAILABLE) Dynamical system initial condition update hook
         */
        void onModelUpdate(const cvec<Tnx> /*x0*/)
        {
        }

    private:
        ProblemBuilder<MPCSize(Tnx, Tnu, Tndu, Tny, Tph, Tch, 0, 0)> builder;
    };
} // namespace mpc
