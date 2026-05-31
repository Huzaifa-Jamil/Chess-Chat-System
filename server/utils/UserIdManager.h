class UserIdManager
{
private:
    int userId;

public:
    UserIdManager()
    {
        userId = 1;
    }

    int assignNextUserId()
    {
        return userId++;
    }
};