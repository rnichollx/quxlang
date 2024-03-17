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

    /**
     * A specialization of question_impl_traits should implement:
     *
     * question_tag (pure_question_tag or impure_question_tag)
     *
     * template <typename Co>
     * Co co_process(G & graph, question_traits<Q>::input_type input);
     *
     * */
    template <typename G, typename Q>
    struct question_impl_traits;


    class question_graph
    {
        virtual ~question_graph()
        {
        }
    };


    template <typename Q>
    class static_answerer
        : public virtual question_graph
    {
        std::map< question_traits< Q >::input_type, question_traits< Q >::output_type > m_answers;

        friend class question_impl_traits< static_answerer< Q >, Q >;

    public:
        template <typename It>
        static_answerer(It begin, It end)
        {

            for (auto it = begin; it != end; ++it)
            {
                m_answers[it->first] = it->second;
            }
        }

    };

    template <typename Q>
    struct question_impl_traits< static_answerer< Q >, Q >
    {
        using question_tag = impure_question_tag;

        template <typename Co>
        Co co_process(static_answerer< Q > & graph, question_traits< Q >::input_type input)
        {
            co_return graph.m_answers[input];
        }
    };

    class question_coroutine
    {
    public:
        class promise_type
        {
        };
    };
}
#endif //RESOLVER2_HPP