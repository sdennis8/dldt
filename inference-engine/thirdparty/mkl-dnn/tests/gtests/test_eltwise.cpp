/*******************************************************************************
* Copyright 2016-2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "gtest/gtest.h"
#include "mkldnn_test_common.hpp"
#include "math_utils.hpp"
#include "mkldnn.hpp"

using namespace mkldnn::impl::math;

namespace mkldnn {

template <typename data_t>
struct eltwise_test_params {
    engine::kind engine_kind;
    algorithm alg_kind;
    memory::format data_format;
    memory::format diff_format;
    data_t alpha, beta;
    memory::dims dims;
    bool expect_to_fail;
    mkldnn_status_t expected_status;
};

size_t n_elems(const memory::desc &md) {
    size_t p = 1;
    const ptrdiff_t *pdims = md.data.layout_desc.blocking.padding_dims;
    for (int i = 0; i < md.data.ndims; ++i)
        p *= (size_t)(pdims[i]);
    return p;
}

template <typename data_t>
void check_eltwise_fwd(const eltwise_test_params<data_t> &p,
        const memory::desc &md, const memory &src, const memory &dst)
{
    data_t *src_data = (data_t *)src.get_data_handle();
    data_t *dst_data = (data_t *)dst.get_data_handle();

    ASSERT_EQ(md.data.data_type, memory::data_type::f32); // TODO: type assert

    size_t n = n_elems(md);
    for (size_t i = 0; i < n; ++i) {
        data_t s = src_data[i];
        data_t ref_d = 0;
        switch (p.alg_kind) {
        case eltwise_relu:        ref_d = relu_fwd(s, p.alpha);           break;
        case eltwise_tanh:        ref_d = tanh_fwd(s);                    break;
        case eltwise_elu:         ref_d = elu_fwd(s, p.alpha);            break;
        case eltwise_square:      ref_d = square_fwd(s);                  break;
        case eltwise_abs:         ref_d = abs_fwd(s);                     break;
        case eltwise_sqrt:        ref_d = sqrt_fwd(s);                    break;
        case eltwise_linear:      ref_d = linear_fwd(s, p.alpha, p.beta); break;
        case eltwise_bounded_relu: ref_d = bounded_relu_fwd(s, p.alpha);  break;
        case eltwise_soft_relu:   ref_d = soft_relu_fwd(s);               break;
        case eltwise_logistic:    ref_d = logistic_fwd(s);                break;
        case eltwise_clamp:       ref_d = clamp_fwd(s, p.alpha, p.beta);  break;
        case eltwise_exp:         ref_d = exp_fwd(s);                     break;
        case eltwise_not:         ref_d = not_fwd(s);                     break;
        default: assert(!"unknown alg_kind");
        }
        dst_data[i] = ref_d;
    }
}

template <typename data_t>
void compare_eltwise_fwd(const eltwise_test_params<data_t> &p,
        const memory::desc &md, const memory &dst, const memory &ref_dst)
{
    data_t *ref_dst_data = (data_t *)ref_dst.get_data_handle();
    data_t *dst_data = (data_t *)dst.get_data_handle();

    ASSERT_EQ(md.data.data_type, memory::data_type::f32); // TODO: type assert

    size_t n = n_elems(md);
    for (size_t i = 0; i < n; ++i) {
        if (p.alg_kind == eltwise_soft_relu){
            EXPECT_NEAR(dst_data[i], ref_dst_data[i], 2.e-6);
        }
        else{
            EXPECT_NEAR(dst_data[i], ref_dst_data[i], 1.e-6);
        }
    }
}


template <typename data_t>
void check_eltwise_bwd(const eltwise_test_params<data_t> &p,
        const memory::desc &md, const memory &src, const memory &diff_dst,
        const memory &diff_src)
{
    data_t *src_data = (data_t *)src.get_data_handle();
    data_t *diff_dst_data = (data_t *)diff_dst.get_data_handle();
    data_t *diff_src_data = (data_t *)diff_src.get_data_handle();

    const memory::desc data_d = src.get_primitive_desc().desc();
    const memory::desc diff_data_d = diff_src.get_primitive_desc().desc();

    ASSERT_EQ(md.data.data_type, memory::data_type::f32); // TODO: type assert

    size_t n = n_elems(md);
    for (size_t i = 0; i < n; ++i) {
        data_t ref_s = src_data[map_index(data_d, i)];
        data_t ref_dd = diff_dst_data[map_index(diff_data_d, i)];
        data_t ref_ds = 0;
        switch (p.alg_kind) {
        case eltwise_relu:   ref_ds = relu_bwd(ref_dd, ref_s, p.alpha); break;
        case eltwise_tanh:   ref_ds = tanh_bwd(ref_dd, ref_s); break;
        case eltwise_elu:    ref_ds = elu_bwd(ref_dd, ref_s, p.alpha); break;
        case eltwise_square: ref_ds = square_bwd(ref_dd, ref_s); break;
        case eltwise_abs:    ref_ds = abs_bwd(ref_dd, ref_s); break;
        case eltwise_sqrt:   ref_ds = sqrt_bwd(ref_dd, ref_s); break;
        case eltwise_linear:
            ref_ds = linear_bwd(ref_dd, ref_s, p.alpha, p.beta);
            break;
        case eltwise_bounded_relu:
            ref_ds = bounded_relu_bwd(ref_dd, ref_s, p.alpha);
            break;
        case eltwise_soft_relu:
            ref_ds = soft_relu_bwd(ref_dd, ref_s);
            break;
        case eltwise_logistic: ref_ds = logistic_bwd(ref_dd, ref_s); break;
        case eltwise_clamp: ref_ds = clamp_bwd(ref_dd, ref_s, p.alpha, p.beta); break;
        case eltwise_exp: ref_ds = exp_bwd(ref_dd, ref_s); break;
        default: assert(!"unknown alg_kind");
        }
        EXPECT_NEAR(diff_src_data[map_index(diff_data_d, i)], ref_ds, 1.e-6);
    }
}

template <typename data_t>
class eltwise_test : public ::testing::TestWithParam<eltwise_test_params<data_t>> {
private:
    std::shared_ptr<memory> src;
    std::shared_ptr<memory> diff_src;
    std::shared_ptr<memory> dst;
    std::shared_ptr<memory> ref_dst;
    std::shared_ptr<memory> diff_dst;
    std::shared_ptr<memory> workspace;
    std::shared_ptr<memory::desc> data_desc;
    std::shared_ptr<memory::desc> diff_data_desc;
    std::shared_ptr<eltwise_forward::primitive_desc> eltwise_prim_desc;
    eltwise_test_params<data_t> p;
    std::shared_ptr<engine> eng;
    memory::data_type data_type;

protected:
    virtual void SetUp() {
        p = ::testing::TestWithParam<decltype(p)>::GetParam();
        catch_expected_failures([=](){Test();}, p.expect_to_fail,
                    p.expected_status);
    }

    void Test() {
        p = ::testing::TestWithParam<eltwise_test_params<data_t>>::GetParam();

        ASSERT_TRUE(p.engine_kind == engine::kind::cpu);
        eng.reset(new engine(p.engine_kind, 0));

        data_type = data_traits<data_t>::data_type;
        ASSERT_EQ(data_type, mkldnn::memory::data_type::f32);

        Forward();
        Backward();
    }

    void Forward() {
        data_desc.reset(new memory::desc(p.dims, data_type,
            p.data_format));
        diff_data_desc.reset(new memory::desc(p.dims, data_type,
            p.diff_format));
        src.reset(new memory({*data_desc, *eng}));
        dst.reset(new memory({*data_desc, *eng}));
        ref_dst.reset(new memory({*data_desc, *eng}));

        data_t data_median = data_t(0);
        data_t data_deviation
                = p.alg_kind == eltwise_elu || p.alg_kind == eltwise_exp ? data_t(1) : data_t(200);
        fill_data<data_t>(n_elems(*data_desc), (data_t *)src->get_data_handle(),
                data_median, data_deviation);
        check_zero_tail<data_t>(1, *src);

        auto eltwise_desc = eltwise_forward::desc(prop_kind::forward_training,
                p.alg_kind, *data_desc, p.alpha, p.beta);
        eltwise_prim_desc.reset(
                new eltwise_forward::primitive_desc(eltwise_desc, *eng));
        auto eltwise = eltwise_forward(*eltwise_prim_desc, *src, *dst);

        std::vector<primitive> pipeline;
        pipeline.push_back(eltwise);
        auto s = stream(stream::kind::lazy);
        s.submit(pipeline).wait();
        check_zero_tail<data_t>(0, *dst);
        check_eltwise_fwd(p, *data_desc, *src, *ref_dst);
        check_zero_tail<data_t>(1, *ref_dst);
        compare_eltwise_fwd(p, *data_desc, *dst, *ref_dst);

    }

    void Backward() {
        diff_src.reset(new memory({*diff_data_desc, *eng}));
        diff_dst.reset(new memory({*diff_data_desc, *eng}));

        data_t data_median = data_t(0);
        data_t data_deviation
                = p.alg_kind == eltwise_elu ? data_t(1) : data_t(200);
        fill_data<data_t>(n_elems(*diff_data_desc),
                (data_t *)diff_dst->get_data_handle(), data_median,
                data_deviation);
        check_zero_tail<data_t>(1, *diff_dst);

        auto eltwise_bwd_desc = eltwise_backward::desc(p.alg_kind,
                *diff_data_desc, *data_desc, p.alpha, p.beta);
        auto eltwise_bwd_prim_desc = eltwise_backward::primitive_desc(
                eltwise_bwd_desc, *eng, *eltwise_prim_desc);
        auto eltwise_bwd = eltwise_backward(eltwise_bwd_prim_desc, *src,
                *diff_dst, *diff_src);

        std::vector<primitive> pipeline;
        pipeline.push_back(eltwise_bwd);
        auto s = stream(stream::kind::lazy);
        s.submit(pipeline).wait();

        check_zero_tail<data_t>(0, *diff_src);
        check_eltwise_bwd(p, *data_desc, *src, *diff_dst, *diff_src);
    }
};

using eltwise_test_float = eltwise_test<float>;
using eltwise_test_params_float = eltwise_test_params<float>;

TEST_P(eltwise_test_float, TestsEltwise)
{
}

#define EXPAND(args) args

#define EXPAND_FORMATS(data) memory::format::data
#define EXPAND_DIMS(...) { __VA_ARGS__ }

#define ENGINE engine::kind::cpu

#define PARAMS(alg, data, diff_data, alpha, beta, ...) \
    eltwise_test_params_float { ENGINE, algorithm::alg, \
    EXPAND_FORMATS(data), EXPAND_FORMATS(diff_data), \
    alpha, beta, EXPAND_DIMS(__VA_ARGS__) }

#define PARAMS_ALL_ALG(...) \
    EXPAND(PARAMS(eltwise_relu, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_tanh, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_elu, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_square, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_abs, __VA_ARGS__))


#define PARAMS_ALL_ALG_SDPART(...) \
    EXPAND(PARAMS(eltwise_sqrt, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_linear, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_soft_relu, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_bounded_relu, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_logistic, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_clamp, __VA_ARGS__)), \
    EXPAND(PARAMS(eltwise_exp, __VA_ARGS__))


#define INST_TEST_CASE(str, ...) INSTANTIATE_TEST_CASE_P( \
        str, eltwise_test_float, ::testing::Values(__VA_ARGS__))

INST_TEST_CASE(SimpleZeroDim,
    PARAMS_ALL_ALG(ncdhw, nCdhw8c, 0.1f, 0.f, 0, 2, 4, 4, 4),
    PARAMS_ALL_ALG(ncdhw, nCdhw8c, 0.1f, 0.f, 2, 0, 4, 4, 4),
    PARAMS_ALL_ALG_SDPART(nCdhw16c, nCdhw16c, 0.1f, 0.2f, 0, 4, 2, 2, 2),
    PARAMS_ALL_ALG_SDPART(nCdhw16c, nCdhw16c, 0.1f, 0.2f, 4, 0, 2, 2, 2)
);

#define CASE_EF(alg, d0, d1, d2, d3) \
        eltwise_test_params_float { ENGINE, algorithm::eltwise_##alg, \
        EXPAND_FORMATS(nchw), EXPAND_FORMATS(nchw), 0.f, 0.f, {d0, d1, d2, d3}, \
        true, mkldnn_invalid_arguments }
INST_TEST_CASE(SimpleExpectedFails,
    CASE_EF(relu, -1, 2, 4, 4),
    CASE_EF(sqrt, -1, 2, 4, 4),
    CASE_EF(logistic, -1, 2, 4, 4),
    CASE_EF(relu, 1, -2, 4, 4),
    CASE_EF(sqrt, 1, -2, 4, 4),
    CASE_EF(logistic, 1, -2, 4, 4)
);

INST_TEST_CASE(Simple_3D,
    PARAMS_ALL_ALG(ncdhw, nCdhw8c, 0.1f, 0.f, 2, 8, 4, 4, 4),
    PARAMS_ALL_ALG(nCdhw8c, ncdhw, 0.1f, 0.f, 2, 16, 4, 4, 4),
    PARAMS_ALL_ALG(ncdhw, ncdhw, 0.1f, 0.f, 2, 16, 8, 8, 8),
    PARAMS_ALL_ALG(nCdhw8c, nCdhw8c, 0.1f, 0.f, 2, 16, 16, 8, 6),
    PARAMS_ALL_ALG(ndhwc, ncdhw, 0.1f, 0.f, 2, 16, 10, 8, 6),
    PARAMS_ALL_ALG(ncdhw, ndhwc, 0.1f, 0.f, 10, 10, 10, 10, 10)
);

INST_TEST_CASE(Simple_blocked_3d_padded,
    PARAMS_ALL_ALG(nCdhw16c, nCdhw16c, 0.1f, 0.2f, 4, 15, 2, 2, 2),
    PARAMS_ALL_ALG_SDPART(nCdhw16c, nCdhw16c, 0.1f, 0.2f, 4, 27, 2, 2, 2),
    PARAMS_ALL_ALG(nCdhw16c, nCdhw16c, 0.1f, 0.2f, 4, 23, 2, 2, 2),
    PARAMS_ALL_ALG_SDPART(nCdhw16c, nCdhw16c, 0.1f, 0.2f, 4, 23, 7, 7, 7)
);

INST_TEST_CASE(Simple_blocked_padded,
    PARAMS_ALL_ALG(nChw16c, nChw16c, 0.1f, 0.2f, 4, 15, 2, 2),
    PARAMS_ALL_ALG_SDPART(nChw16c, nChw16c, 0.1f, 0.2f, 4, 27, 2, 2),
    PARAMS_ALL_ALG(nChw16c, nChw16c, 0.1f, 0.2f, 4, 23, 2, 2),
    PARAMS_ALL_ALG_SDPART(nChw16c, nChw16c, 0.1f, 0.2f, 4, 17, 7, 7),
    PARAMS_ALL_ALG(nChw8c, nChw8c, 0.1f, 0.2f, 4, 15, 2, 2),
    PARAMS_ALL_ALG_SDPART(nChw8c, nChw8c, 0.1f, 0.2f, 4, 27, 2, 2),
    PARAMS_ALL_ALG(nChw8c, nChw8c, 0.1f, 0.2f, 4, 23, 2, 2),
    PARAMS_ALL_ALG_SDPART(nChw8c, nChw8c, 0.1f, 0.2f, 4, 17, 7, 7)
);

INST_TEST_CASE(Simple_NCDHW,
    PARAMS_ALL_ALG(ncdhw, ncdhw, 0.f, 0.f, 2, 32, 28, 28, 28),
    PARAMS_ALL_ALG(ncdhw, ncdhw, 1.f, 0.f, 2, 64, 13, 13, 13),
    PARAMS_ALL_ALG(ncdhw, ncdhw, 1.f, 1.f, 1, 64, 27, 27, 27),
    PARAMS_ALL_ALG(ncdhw, ncdhw, 0.f, 1.f, 1, 128, 11, 11, 11)
);

INST_TEST_CASE(SimpleZeroNegativeSlope_NCHW,
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 8, 4, 4),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 16, 4, 4),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 16, 8, 8),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 16, 16, 8),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 16, 10, 8),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 10, 10, 10, 10),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 256, 64, 8, 16),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 1, 1, 1, 1),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 3, 5, 7, 11)
);

INST_TEST_CASE(Simple_NCHW,
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 2, 8, 4, 4),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 2, 16, 4, 4),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 2, 16, 8, 8),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 2, 16, 16, 8),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 2, 16, 10, 8),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 10, 10, 10, 10),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 256, 64, 8, 16),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 1, 1, 1, 1),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 3, 5, 7, 11)
);

INST_TEST_CASE(Simple_NCHW_SDPART,
    PARAMS_ALL_ALG_SDPART(nchw, nchw, 0.1f, 0.f, 256, 64, 8, 16)
);

INST_TEST_CASE(Simple,
    PARAMS_ALL_ALG(nchw, nChw8c, 0.1f, 0.f, 2, 8, 4, 4),
    PARAMS_ALL_ALG(nChw8c, nchw, 0.1f, 0.f, 2, 16, 4, 4),
    PARAMS_ALL_ALG(nchw, nchw, 0.1f, 0.f, 2, 16, 8, 8),
    PARAMS_ALL_ALG(nChw8c, nChw8c, 0.1f, 0.f, 2, 16, 16, 8),
    PARAMS_ALL_ALG(nhwc, nchw, 0.1f, 0.f, 2, 16, 10, 8),
    PARAMS_ALL_ALG(nchw, nhwc, 0.1f, 0.f, 10, 10, 10, 10)
);

INST_TEST_CASE(Simple_SDPART,
    PARAMS_ALL_ALG_SDPART(nchw, nChw8c, 0.1f, 0.f, 2, 8, 4, 4),
    PARAMS_ALL_ALG_SDPART(nChw8c, nchw, 0.1f, 0.f, 2, 16, 4, 4),
    PARAMS_ALL_ALG_SDPART(nchw, nchw, 0.1f, 0.f, 2, 16, 8, 8),
    PARAMS_ALL_ALG_SDPART(nChw8c, nChw8c, 0.1f, 0.f, 2, 16, 16, 8),
    PARAMS_ALL_ALG_SDPART(nhwc, nchw, 0.1f, 0.f, 2, 16, 10, 8),
    PARAMS_ALL_ALG_SDPART(nchw, nhwc, 0.1f, 0.f, 10, 10, 10, 10)
);

INST_TEST_CASE(AlexNet_NCHW,
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 96, 55, 55),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 256, 27, 27),
    PARAMS_ALL_ALG(nchw, nchw, 0.f, 0.f, 2, 384, 13, 13),
    PARAMS_ALL_ALG_SDPART(nchw, nchw, 0.f, 0.f, 2, 96, 55, 55),
    PARAMS_ALL_ALG_SDPART(nchw, nchw, 0.f, 0.f, 2, 256, 27, 27),
    PARAMS_ALL_ALG_SDPART(nchw, nchw, 0.f, 0.f, 2, 384, 13, 13)
);

INST_TEST_CASE(Simple_X,
    PARAMS_ALL_ALG(x, x, 0.f, 0.f, 55)
);

}
