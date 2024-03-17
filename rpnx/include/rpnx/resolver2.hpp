// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef RESOLVER2_HPP
#define RESOLVER2_HPP

namespace rpnx
{

    // A pure question is a question that depends only on the input and
    // any sub-questions it may ask.
    // It can be cached between runs if the executable version is the same.
    // Note the dependencies don't have to be pure, only the question itself.
    // Example: "class_placement_info_question"
    struct pure_question_tag
    {
    };

    // An impure question depends upon the state of the system,
    // other than the input and sub-questions.
    // Cannot be cached between runs.
    // Example: "input_file_list_question"
    struct impure_question_tag
    {
    };


    /**
     * A specialization of question-traits should implement:
     *  input_type
     *  output_type
     *  name (string)
     *  */
    template <typename T>
    struct question_traits;

    template <typename G, typename Q>
    struct question_impl_traits;

    namespace detail
    {
        struct askable_state
        {

        };
    }


}

#endif //RESOLVER2_HPP