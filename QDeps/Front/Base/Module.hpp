struct My : Front::Base::Module
    <
        Externals::Bind         < MyClass, Named<int, dupa>                                             >,
        Scope<Singleton >::Bind < Impl1, Bind<Interface, Impl2>, Bind< InCall<C1, C2>, Impl2>           >,
        Scope<PerRequest>::Bind < int_<32>, mpl::string<'hej'>, Bind<Named<int, dupa>, int_<42> >       >
    >
{ };

Impl<PerRequest, CIf0, CIf0, vector0<>, boost::is_base_of<boost::mpl::_1, CIf0> >
