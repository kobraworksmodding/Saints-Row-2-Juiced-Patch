module;

#include <vector>
#include <functional>

export module component;

export class Juiced
{
public:
    template <typename... Args>
    class Call
    {
    public:
        Call& operator+=(std::function<void(Args...)> cb)
        {
            callbacks_.push_back(std::move(cb));
            return *this;
        }

        void addCallback(std::function<void(Args...)> cb)
        {
            callbacks_.push_back(std::move(cb));
        }

        void executeAll(Args... args)
        {
            for (auto& cb : callbacks_)
            {
                cb(args...);
            }
        }

    private:
        std::vector<std::function<void(Args...)>> callbacks_;
    };

public:
    static Call<>& onAttach() {
        static Call<> onProcessAttach;
        return onProcessAttach;
    }
    static Call<>& onGameInitStage_1() {
        static Call<> onGameInitStage_1;
        return onGameInitStage_1;
    }
    static Call<>& onGameInitStage_2() {
        static Call<> onGameInitStage_2;
        return onGameInitStage_2;
    }
    static Call<>& onInputPoll() {
        static Call<> onInputPoll;
        return onInputPoll;
    }
};