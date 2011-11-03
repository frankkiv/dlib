// Copyright (C) 2011  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_STRUCTURAL_SEQUENCE_LABELING_TRAiNER_H__
#define DLIB_STRUCTURAL_SEQUENCE_LABELING_TRAiNER_H__

#include "structural_sequence_labeling_trainer_abstract.h"
#include "../algs.h"
#include "../optimization.h"
#include "structural_svm_sequence_labeling_problem.h"


namespace dlib
{

// ----------------------------------------------------------------------------------------

    template <
        typename feature_extractor
        >
    class structural_sequence_labeling_trainer
    {
    public:
        typedef typename feature_extractor::sample_type sample_type;
        typedef std::vector<sample_type> sample_sequence_type;
        typedef std::vector<unsigned long> labeled_sequence_type;

        typedef sequence_labeler<feature_extractor> trained_function_type;

        explicit structural_sequence_labeling_trainer (
            const feature_extractor& fe_
        ) : fe(fe_)
        {
            set_defaults();
        }

        structural_sequence_labeling_trainer (
        )
        {
            set_defaults();
        }

        const feature_extractor& get_feature_extractor (
        ) const { return fe; }

        unsigned long num_labels (
        ) const { return fe.num_labels(); }

        void set_num_threads (
            unsigned long num
        )
        {
            num_threads = num;
        }

        unsigned long get_num_threads (
        ) const
        {
            return num_threads;
        }

        void set_epsilon (
            double eps_
        )
        {
            // make sure requires clause is not broken
            DLIB_ASSERT(eps_ > 0,
                "\t void structural_sequence_labeling_trainer::set_epsilon()"
                << "\n\t eps_ must be greater than 0"
                << "\n\t eps_: " << eps_ 
                << "\n\t this: " << this
                );

            eps = eps_;
        }

        const double get_epsilon (
        ) const { return eps; }

        void set_max_cache_size (
            unsigned long max_size
        )
        {
            max_cache_size = max_size;
        }

        unsigned long get_max_cache_size (
        ) const
        {
            return max_cache_size; 
        }

        void be_verbose (
        )
        {
            verbose = true;
        }

        void be_quiet (
        )
        {
            verbose = false;
        }

        void set_oca (
            const oca& item
        )
        {
            solver = item;
        }

        const oca get_oca (
        ) const
        {
            return solver;
        }

        void set_c (
            double C_ 
        )
        {
            // make sure requires clause is not broken
            DLIB_ASSERT(C_ > 0,
                "\t void structural_sequence_labeling_trainer::set_c()"
                << "\n\t C_ must be greater than 0"
                << "\n\t C_:    " << C_ 
                << "\n\t this: " << this
                );

            C = C_;
        }

        double get_c (
        ) const
        {
            return C;
        }


        const sequence_labeler<feature_extractor> train(
            const std::vector<sample_sequence_type>& x,
            const std::vector<labeled_sequence_type>& y
        ) const
        {

            // make sure requires clause is not broken
            DLIB_ASSERT(is_sequence_labeling_problem(x,y) == true,
                        "\t sequence_labeler structural_sequence_labeling_trainer::train(x,y)"
                        << "\n\t invalid inputs were given to this function"
                        << "\n\t x.size(): " << x.size() 
                        << "\n\t is_sequence_labeling_problem(x,y): " << is_sequence_labeling_problem(x,y)
            );

#ifdef ENABLE_ASSERTS
            for (unsigned long i = 0; i < y.size(); ++i)
            {
                for (unsigned long j = 0; j < y[i].size(); ++j)
                {
                    // make sure requires clause is not broken
                    DLIB_ASSERT(y[i][j] < num_labels(),
                                "\t sequence_labeler structural_sequence_labeling_trainer::train(x,y)"
                                << "\n\t The given labels in y are invalid."
                                << "\n\t y[i][j]: " << y[i][j] 
                                << "\n\t num_labels(): " << num_labels()
                                << "\n\t i: " << i 
                                << "\n\t j: " << j 
                                << "\n\t this: " << this
                    );
                }
            }
#endif




            structural_svm_sequence_labeling_problem<feature_extractor> prob(x, y, fe, num_threads);
            matrix<double,0,1> weights; 
            if (verbose)
                prob.be_verbose();

            prob.set_epsilon(eps);
            prob.set_c(C);
            prob.set_max_cache_size(max_cache_size);
            solver(prob, weights);

            return sequence_labeler<feature_extractor>(fe,weights);
        }

    private:

        double C;
        oca solver;
        double eps;
        bool verbose;
        unsigned long num_threads;
        unsigned long max_cache_size;

        void set_defaults ()
        {
            C = 100;
            verbose = false;
            eps = 0.1;
            num_threads = 2;
            max_cache_size = 40;
        }

        feature_extractor fe;
    };

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_STRUCTURAL_SEQUENCE_LABELING_TRAiNER_H__


