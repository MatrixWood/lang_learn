def user_login_data(f):
    def wrapper(*args, **kwargs):
        return f(*args, **kwargs)
    wrapper._test = "test123"
    return wrapper

@user_login_data
def num1():
    print("aaa")

print(num1._test)